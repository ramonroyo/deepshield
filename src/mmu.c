#include "dsdef.h"
#include "mmu.h"
#include "smp.h"
#include "mem.h"
#include "os.h"

VOID
MmuFinalize(
    VOID
);

#define HIGHEST_PHYSICAL_ADDRESS   ((ULONG64)~((ULONG64)0))

MMU gMmu = { 0 };

PTE DefaultMmuPte = { PTE_PRESENT |
                      PTE_RW |
                      // PTE_GLOBAL |
                      PTE_DIRTY |
                      PTE_ACCESSED };

#if DBG
NTSTATUS 
MmuLocatePageTables(
    _Inout_ PULONG_PTR PdeBase,
    _Inout_ PULONG_PTR PteBase
    )
{
    UNICODE_STRING RoutineName = RTL_CONSTANT_STRING( L"ExFreePoolWithTag" );
    PUCHAR RoutineAddress = MmGetSystemRoutineAddress( &RoutineName );

    if ( RoutineAddress ) {

        *PdeBase = *(PULONG_PTR)(RoutineAddress + 0x32D + 2);
        *PteBase = *(PULONG_PTR)(RoutineAddress + 0x6A9 + 2);

        return STATUS_SUCCESS;
    }

    return STATUS_NOT_FOUND;
}
#endif

_Success_(return)
BOOLEAN
MmupLocateAutoEntryIndex(
    _Out_ PUINT64 autoEntryIndex
)
{
    UINT64 cr3   = 0;
    PHYSICAL_ADDRESS cr3Pa = { 0 };
    PVOID cr3Va = 0;
    BOOLEAN found = FALSE;

    cr3 = __readcr3() & 0xFFFFFFFFFFFFF000;
    cr3Pa.QuadPart = cr3;

#if (NTDDI_VERSION >= NTDDI_VISTA)
    if (!RtlIsNtDdiVersionAvailable( NTDDI_WIN10 )) {
#endif
        cr3Va = MmMapIoSpace( cr3Pa, PAGE_SIZE, MmNonCached );

#if (NTDDI_VERSION >= NTDDI_VISTA)
    } else {
        cr3Va = DsMmMapIoSpaceEx( cr3Pa,
                                  PAGE_SIZE, 
                                  PAGE_READWRITE | PAGE_NOCACHE );
    }
#endif

    if (cr3Va)
    {
        PUINT64 pml4 = (PUINT64)cr3Va;
        UINT16  i;

        for (i = 0; i < 512; i++) {

            UINT64 entry = pml4[i];
            if ((entry & 0x7FFFFFFFFFFFF000) == cr3) {

                *autoEntryIndex = i;
                found = TRUE;
                break;
            }
        }

        MmUnmapIoSpace( cr3Va, PAGE_SIZE );
    }
   
    return found;
}

#define PDPTE_PFN_MASK    0x0000FFFFFFFFF000

UINT64
MmupCreatePte(
    _In_ PHYSICAL_ADDRESS PhysicalAddress,
    _In_ UINT64 PteContents
    )
{
#ifdef _WIN64
    return (PhysicalAddress.QuadPart & PDPTE_PFN_MASK) | PteContents;

#else
    if (gMmu.PaeEnabled) {
        return (PhysicalAddress.QuadPart & PDPTE_PFN_MASK) | PteContents;
    }
    else {
        return (PhysicalAddress.LowPart & 0xFFFFF000) | (UINT32)PteContents;
    }
#endif
}

VOID
MmupWriteMappingPte(
    _In_ PMMU_MAPPING Mapping,
    _In_ UINT64 PteContents
    )
{
#ifdef _WIN64
    *(PUINT64)Mapping->PointerPte = PteContents;

#else
    if (gMmu.PaeEnabled) {
        *(PUINT64)Mapping->PointerPte = PteContents;
    }
    else {
        *(PUINT32)Mapping->PointerPte = (UINT32)PteContents;
    }
#endif
    
    //
    //  Flush the address from TLB for the current processor. 
    //
    __invlpg( Mapping->SystemVa );
}

typedef union _VIRTUAL_ADDRESS
{
    struct
    {
        UINT64 Offset : 12;
        UINT64 PtIndex : 9;
        UINT64 PdIndex : 9;
        UINT64 PdptIndex : 9;
        UINT64 Pml4Index : 9;
        UINT64 Reserved : 16;
    };

    PVOID AsPointer;
} VIRTUAL_ADDRESS;

typedef struct _PTE_MAPPER
{
    VIRTUAL_ADDRESS MappingPage;
    PHYSICAL_ADDRESS AllocatedAddress;
    PPTE MappingPte;
} PTE_MAPPER, *PPTE_MAPPER;

static PTE_MAPPER gMapper;
static PMDL MdlArray[32] = { NULL };
static CHAR MappingSpace[PAGE_SIZE * 32] = { 0 };

#define PAGE_MASK (~(PAGE_SIZE-1))

#define PFN_TO_PAGE(pfn) (pfn << PAGE_SHIFT)
#define PAGE_TO_PFN(pfn) (pfn >> PAGE_SHIFT)

#define PAGE_ALIGN(Va) ((PVOID)((ULONG_PTR)(Va) & ~(PAGE_SIZE - 1)))

PVOID
MmuAllocateMappingPage(
    _In_ UINT32 Index
    ) 
{
    PVOID MappingVa = NULL;
    PMDL *MappingMdl = &MdlArray[Index];

    NT_ASSERT( *MappingMdl == NULL );

    if (Index > 32) {
        //
        //  TODO: Remove when not a prototype.
        //

        return NULL;
    }

    //
    //  Use data section as non-paged pool pages allocated from large pages
    //  are not suitable for PTE remapping.
    //

    *MappingMdl = IoAllocateMdl( PAGE_ALIGN( &MappingSpace[Index] ),
                                       PAGE_SIZE, 
                                       FALSE,
                                       FALSE, 
                                       NULL );

    if (NULL == *MappingMdl) {
		return NULL;
	}

	try {

		//
        //  Lock the physical page into memory to avoid paging I/O.
        //

		MmProbeAndLockPages( *MappingMdl, KernelMode, IoReadAccess );
        MappingVa = MmMapLockedPagesSpecifyCache( *MappingMdl,
                                                  KernelMode,
                                                  MmCached,
                                                  NULL,
                                                  0,
                                                  NormalPagePriority );
	} except( EXCEPTION_EXECUTE_HANDLER ) {

		IoFreeMdl( *MappingMdl );
        *MappingMdl = NULL;
	}

    return MappingVa;
}

VOID
MmuFreeMappingPage(
    _In_ UINT32 Index
    )
{
    PMDL *MappingMdl = &MdlArray[ Index ];

    if (NULL != *MappingMdl) {

        MmUnlockPages( *MappingMdl );
        IoFreeMdl( *MappingMdl );

        *MappingMdl = NULL;
    }
}

#ifdef _WIN64
//
//  TODO: Complete routines and then replace.
//
NTSTATUS
MmuInitializePteMapper(
    VOID
    )
{
    gMapper.MappingPage.AsPointer = MmuAllocateMappingPage( 0 );
    if (gMapper.MappingPage.AsPointer == NULL) {
        return STATUS_NO_MEMORY;
    }

    //
    //  While locked there is no need to traverses the page tables to find
    //  the pte for the mapping page.
    //
    gMapper.MappingPte = (PPTE)MmuGetPteAddress( gMapper.MappingPage.AsPointer );
    return STATUS_SUCCESS;
}

VOID
MmuDeletePteMapper(
    VOID
    )
{
    MmuFreeMappingPage( 0 );
}
#endif

PVOID
MmupMapPage(
    _In_ PHYSICAL_ADDRESS TargetPa,
    _In_ UINT64 PteContents
    )
{
    UINT32 i;
    PMMU_PROCESSOR MmuVcpu;

    MmuVcpu = &gMmu.Processors[SmpGetCurrentProcessor()];

    //
    //  Find an available mapping PTE for this VCPU.
    //

    for (i = 0; i < MAX_MAPPING_SLOTS; i++) {
        if (0 == InterlockedCompareExchange( &MmuVcpu->Mappings[i].MapInUse, 
                                             TRUE,
                                             FALSE ) ) {

            //
            //  Write valid PTE in paging structures so this physical address
            //  becomes accessible.
            //
            MmupWriteMappingPte( &MmuVcpu->Mappings[i],
                               MmupCreatePte( TargetPa, PteContents ) );

            return MmuVcpu->Mappings[i].SystemVa;
        }
    }

    return NULL;
}

NTSTATUS
MmuInitialize(
    VOID
    )
{
    NTSTATUS Status = STATUS_INSUFFICIENT_RESOURCES;
    LARGE_INTEGER MaxAddress = { HIGHEST_PHYSICAL_ADDRESS };
    UINT32 ProcessorCount = SmpActiveProcessorCount();
    UINT64 PteContents;
    PVOID ZeroPage;
    PMMU_MAPPING Mapping;
    UINT32 i, j;

#ifdef _WIN64
#if (NTDDI_VERSION >= NTDDI_VISTA)
    PKD_DEBUGGER_DATA_BLOCK DebuggerData;
    
    if (gSecuredPageTables) {

#if DBG
        ULONG_PTR PdeBase;
        ULONG_PTR PteBase;

        if (OsVerifyBuildNumber( DS_WINVER_10_RS4 )) {
            Status = MmuLocatePageTables( &PdeBase, &PteBase );
            NT_ASSERT( NT_SUCCESS( Status ) );
        }
#endif

        Status = OsGetDebuggerDataBlock( &DebuggerData );
        if (!NT_SUCCESS( Status )) {
            return Status;
        }

        gMmu.lowerBound = DebuggerData->PteBase;
        gMmu.upperBound = (gMmu.lowerBound + 0x8000000000 - 1) & 0xFFFFFFFFFFFFFFF8;

    } else {
#endif
        //
        //  Find processor page structures in memory as now they are ASLR'd.
        //
        if (!MmupLocateAutoEntryIndex( &gMmu.autoEntryIndex )) {
            return STATUS_UNSUCCESSFUL;
        }

        gMmu.lowerBound =  (((UINT64)0xFFFF) << 48) | (gMmu.autoEntryIndex << 39);
        gMmu.upperBound = (gMmu.lowerBound + 0x8000000000 - 1) & 0xFFFFFFFFFFFFFFF8;

#if (NTDDI_VERSION >= NTDDI_VISTA)
    }
#endif

#else // !WIN64
    ULONG_PTR cr4 = 0;

    cr4 = __readcr4();
    if (cr4 & CR4_PAE_ENABLED)
    {
        gMmu.PaeEnabled = TRUE;
        gMmu.PxeShift = 3;
    }
    else
    {
        gMmu.PaeEnabled = FALSE;
        gMmu.PxeShift = 2;
    }
#endif
    gMmu.PxeBase = (UINTN)MmuGetPteAddress( 0 );

    //
    //  Allocate one MMU per VCPU.
    //
    gMmu.Processors = MemAllocArray( ProcessorCount, sizeof( MMU_PROCESSOR ));
    
    if (!gMmu.Processors) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory( gMmu.Processors, ProcessorCount * sizeof( MMU_PROCESSOR ));

    for (i = 0; i < ProcessorCount; i++) {

        ZeroPage = MmAllocateContiguousMemory( PAGE_SIZE, MaxAddress );

        if (NULL == ZeroPage) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto RoutineExit;
        }

        gMmu.Processors[i].ZeroPte = MmuGetPteAddress( ZeroPage );
        gMmu.Processors[i].ZeroPage = ZeroPage;

        for (j = 0; j < MAX_MAPPING_SLOTS; j++) {

            PteContents = *(PUINT64)gMmu.Processors[i].ZeroPte;
            Mapping = &gMmu.Processors[i].Mappings[j];

            Mapping->SystemVa = MmuAllocateMappingPage( j + (i * j) );

            if (NULL == Mapping->SystemVa) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto RoutineExit;
            }

            Mapping->PointerPte = MmuGetPteAddress( Mapping->SystemVa );
            Mapping->MapInUse = FALSE;

            MmupWriteMappingPte( Mapping, PteContents );
        }
    }

    Status = STATUS_SUCCESS;

RoutineExit:

    if (!NT_SUCCESS(Status)) {
        MmuFinalize();
    }

    return Status;
}

VOID
MmuFinalize(
    VOID
    )
{
    PMMU_PROCESSOR MmuVcpu;
    UINT32 i, j;

    for (i = 0; i < SmpActiveProcessorCount(); i++) {
        MmuVcpu = &gMmu.Processors[i];

        //
        //  Time to free all address mappings for each VCPU.
        //

        for (j = 0; j < MAX_MAPPING_SLOTS; j++) {

            if (MmuVcpu->Mappings[j].SystemVa) {
                MmupWriteMappingPte( &MmuVcpu->Mappings[j], 0 );
                MmuFreeMappingPage( j + (i * j) );
               // MmFreeMappingAddress( MmuVcpu->Mappings[j].SystemVa,
               //                       'MMAP' );
            }
        }

        if (MmuVcpu->ZeroPage) {
            MmFreeContiguousMemory( MmuVcpu->ZeroPage );
        }
    }

    MemFree( gMmu.Processors );
    gMmu.Processors = 0;

#ifdef _WIN64
    gMmu.lowerBound = 0;
    gMmu.upperBound = 0;
#else
    gMmu.PaeEnabled = FALSE;
    gMmu.PxeShift = 0;
#endif
}

PVOID
MmuMapPage(
    _In_ PHYSICAL_ADDRESS PhysicalPage,
    _In_ BOOLEAN Uncached
    )
{
    UINT64 PteContents;
    BOOLEAN Nxe = FALSE;

    PteContents = DefaultMmuPte.AsUintN;

    if (Nxe) {
        //
        //   TODO: Disabled now but check CR4.NXE.
        //
        PteContents |= PTE_XD;
    }

    if (Uncached) {
        PteContents |= PAGE_CACHE_UC;
    }

    return MmupMapPage( PhysicalPage, PteContents );
}

VOID
MmuUnmapPage(
    _In_ PVOID VirtualAddress
    )
{
    UINT32 i;
    PMMU_PROCESSOR MmuVcpu;
    PMMU_MAPPING Mapping;

    MmuVcpu = &gMmu.Processors[SmpGetCurrentProcessor()];

    for (i = 0; i < MAX_MAPPING_SLOTS; i++) {
        Mapping = &MmuVcpu->Mappings[i];

        //
        //  Look for the matching VirtualAddress to unmap.
        //

        if (Mapping->MapInUse && Mapping->SystemVa == VirtualAddress) {
            InterlockedCompareExchange( &Mapping->MapInUse, 0, 1 );

            MmupWriteMappingPte( Mapping, *(PUINT64)MmuVcpu->ZeroPte );
            break;
        }
    }
}

PVOID
MmuGetPteAddress(
    _In_opt_ PVOID VirtualAddress
    )
{
    UINTN PointerPte = 0;

#ifdef _WIN64
    PointerPte = ((UINT64)VirtualAddress) >> 9;
    PointerPte = PointerPte | gMmu.lowerBound;
    PointerPte = PointerPte & gMmu.upperBound;
#else
    PointerPte = ((UINT32)VirtualAddress) >> 12;
    PointerPte = PointerPte << gMmu.PxeShift;
    PointerPte = PointerPte + 0xC0000000;
#endif

    return (PVOID)PointerPte;
}

PVOID
MmuGetPhysicalAddressMappedByPte(
    _In_ PVOID PxeAddress
    )
{
    UINTN Pfn;
    UINTN PageAddress;

#ifdef _WIN64
        Pfn = ((UINTN)PxeAddress - gMmu.PxeBase) / sizeof( UINT64 );

        PageAddress = Pfn << 12;

        if (PageAddress & ((UINT64)1 << 47)) {
            PageAddress |= ((UINT64)0xFFFF << 48);
        }
#else
    if (gMmu.PaeEnabled) {
        Pfn = ((UINTN)PxeAddress - gMmu.PxeBase) / sizeof( UINT64 );
    }
    else {
        Pfn = ((UINTN)PxeAddress - gMmu.PxeBase) / sizeof( UINT32 );
    }

    PageAddress = Pfn << 12;
#endif

    return (PVOID)PageAddress;
}

BOOLEAN
MmuAddressIsInPagingRange(
    _In_ PVOID address
    )
{
    static UINTN start = 0;
    static UINTN end   = 0;
    
    if(start == 0)
    {
        start = (UINTN)MmuGetPteAddress(0);
        end   = (UINTN)MmuGetPteAddress(((PVOID)((UINTN)-1)));
    }

    return (start <= (UINTN)address) && ((UINTN)address <= end);
}

PHYSICAL_ADDRESS
MmuGetPhysicalAddress(
    _In_ UINTN cr3,
    _In_ PVOID address
    )
{
    PHYSICAL_ADDRESS result;
    UINTN         currentCr3;
    
    cr3        = cr3         & 0xFFFFFFFFFFFFF000;
    currentCr3 = __readcr3() & 0xFFFFFFFFFFFFF000;

    if (cr3 == 0 || cr3 == currentCr3)
    {
        //
        // Mapped page tables corresponds to process
        //
        result = MmGetPhysicalAddress(address);
    }
    else
    {
        //
        // We need to traverse cr3 mapping pages
        //
        PHYSICAL_ADDRESS pa    = { 0 };
        UINT8 level = MmuGetPagingLevels();

        result.QuadPart = 0;
        pa.QuadPart = cr3;

        while(level)
        {
            PVOID pml;
            UINT16 index;
            UINT64 entry;

            pml = MmuMapPage( pa, TRUE );

            //
            // We went out of cached slots, exit
            //
            if ( !pml ) { 
                ASSERT( FALSE );
                break; 
            }

            index = MmuGetPxeIndex( level, address );
            entry = MmuGetPmlEntry( pml, index );

            MmuUnmapPage( pml );

            if (entry & 1) {
                if (level != 1 && (entry & PDE_PSE)) {
                    //
                    //   Check what happens with a 2-MB page.
                    //
                    NT_ASSERT( FALSE );
                }

                if (level == 1) {
                    result.QuadPart = entry & 0xFFFFFFFFFFFFF000 | (((UINTN)address) & 0xFFF);
                }
                else {
                    pa.QuadPart = entry & 0xFFFFFFFFFFFFF000;
                }
            }
            else
            {
                break;
            }
            
            level--;
        }
    }

    return result;

}

UINT16
MmuGetNumberOfPageEntries(
    VOID
)
{
#ifdef _WIN64
    return 512;
#else
    if (gMmu.PaeEnabled)
    {
        return 512;
    }
    else
    {
        return 1024;
    }
#endif
}

UINT8
MmuGetPageEntrySize(
    VOID
)
{
#ifdef _WIN64
    return 8;
#else
    if (gMmu.PaeEnabled)
    {
        return 8;
    }
    else
    {
        return 4;
    }
#endif
}

UINT64
MmuGetPmlEntry(
    _In_ PVOID  pml,
    _In_ UINT16 index
)
{
#ifdef _WIN64
    PUINT64 table;
    table = (PUINT64) pml;
    return table[index];
#else
    if (gMmu.PaeEnabled)
    {
        PUINT64 table;
        table = (PUINT64) pml;
        return table[index];
    }
    else
    {
        PUINT32 table;
        table = (PUINT32)pml;
        return table[index];
    }
#endif
}

UINT16
MmuGetPmlIndexFromOffset(
    _In_ UINT32 offset
)
{
    UINT32 sizeOfEntry;

#ifdef _WIN64
    sizeOfEntry = sizeof(UINTN);
#else
    if (gMmu.PaeEnabled)
    {
        sizeOfEntry = 8;
    }
    else
    {
        sizeOfEntry = sizeof(UINTN);
    }
#endif

    return (UINT16)(offset / sizeOfEntry);
}

UINT8
MmuGetPagingLevels(
    VOID
)
{
#ifdef _WIN64
    return 4;
#else
    if (gMmu.PaeEnabled)
    {
        return  3;
    }
    else
    {
        return  2;
    }
#endif
}

UINT16
MmuGetPxeIndex(
    _In_ UINT8 level,
    _In_ PVOID va
)
{
#ifdef _WIN64
    return  ((((UINTN)va) >> (12 + 9 * (level - 1))) & 0x1FF);
#else
    if (gMmu.PaeEnabled)
    {
        return  ((((UINTN)va) >> (12 + 9 * (level - 1))) & 0x1FF);
    }
    else
    {
        return  ((((UINTN)va) >> (12 + 10 * (level - 1))) & 0x3FF);
    }
#endif
}

BOOLEAN
MmuIsUserModeAddress(
    _In_ PVOID va
)
{
#ifdef _WIN64
    //return (((UINTN)va) & 0x8000000000000000) == 0;
    return ((UINTN)va) <= 0x00007FFFFFFFFFFF;
#else
    return  (((UINTN)va) & 0x80000000) == 0;
#endif
}

BOOLEAN
MmuIsKernelModeAddress(
    _In_ PVOID va
)
{
#ifdef _WIN64
    //return (((UINTN)va) & 0x8000000000000000) == 0x8000000000000000;
    return ((UINTN)va) >= 0xFFFF800000000000;
#else
    return (((UINTN)va) & 0x80000000) == 0x80000000;
#endif
}

BOOLEAN
MmuIsInvalidAddress(
    _In_ PVOID va
)
{
#ifdef _WIN64
    return !MmuIsUserModeAddress(va) && !MmuIsKernelModeAddress(va);
#else
    UNREFERENCED_PARAMETER(va);

    return FALSE;
#endif
}
