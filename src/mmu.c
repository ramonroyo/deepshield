#include "dsdef.h"
#include "mmu.h"
#include "smp.h"
#include "mem.h"
#include "os.h"

MMU gMmu = { 0 };

PTE_ENTRY DefaultMmuPte = { PTE_PRESENT |
                            PTE_RW |
                            // PTE_GLOBAL |
                            PTE_DIRTY |
                            PTE_ACCESSED };

typedef struct _MATCH_ITEM {
    UCHAR Byte;
    UCHAR Mask;
} MATCH_ITEM, *PMATCH_ITEM;

ULONG
MatchSignature(
    PUSHORT Signature,
    ULONG   SignatureSize,
    PUCHAR  Target,
    ULONG   TargetSize
    )
{
    ULONG  SignatureOffset = 0;
    ULONG  TargetOffset = 0;
    ULONG  Offset = 0;
    ULONG  Limit = 0;

    PMATCH_ITEM Item;

    NT_ASSERT( Signature != NULL );
    NT_ASSERT( Target != NULL );

    Limit = TargetSize - SignatureSize;

    for ( TargetOffset = 0; TargetOffset < Limit; TargetOffset++ ) {
        for ( SignatureOffset = 0; SignatureOffset < SignatureSize; SignatureOffset++ ) {

            Item = (PMATCH_ITEM) &Signature[SignatureOffset];

            if ( !((Target[TargetOffset + SignatureOffset] & Item->Mask) == Item->Byte) ) {
                break;
            }
        }

        if ( SignatureOffset == SignatureSize ) {
            Offset = TargetOffset;
            break;
        }
    }

    return Offset;
}

ULONG_PTR
HackishSearchPML4(
    VOID
    )
{

//              MmAccessFault+2E6  48 B8 00 D0 BE 7D FB F6 FF FF mov     rax, 0FFFFF6FB7DBED000h
//              MmAccessFault+2F0  48 8B C0                      mov     rax, rax
//              MmAccessFault+2F3  4C 3B C8                      cmp     r9, rax
//              MmAccessFault+2F6  0F 82 72 FF FF FF             jb      loc_FFFFF803FBE8E74E
//              MmAccessFault+2FC  48 B8 FF DF BE 7D FB F6 FF FF mov     rax, 0FFFFF6FB7DBEDFFFh
//              MmAccessFault+306  48 8B C0                      mov     rax, rax
//              MmAccessFault+309  4C 3B C8                      cmp     r9, rax
//              MmAccessFault+30C  0F 87 5C FF FF FF             ja      loc_FFFFF803FBE8E74E
//              MmAccessFault+312  E9 36 C1 16 00                jmp     sub_FFFFF803FBFFA92D
//              MmAccessFault+317                                ; ---------------------------------------------------------------------------
//              MmAccessFault+317
//              MmAccessFault+317                                loc_FFFFF803FBE8E7F7:                   ; CODE XREF: MmAccessFault+281
//              MmAccessFault+317  48 B8 F8 D7 BE 7D FB F6 FF FF mov     rax, 0FFFFF6FB7DBED7F8h         // PML4 Static Address
//              MmAccessFault+321  48 8B C0                      mov     rax, rax
//              MmAccessFault+324  4C 3B C8                      cmp     r9, rax
//              MmAccessFault+327  0F 87 5A FF FF FF             ja      loc_FFFFF803FBE8E767
//              MmAccessFault+32D  E9 4B C1 16 00                jmp     sub_FFFFF803FBFFA95D
//              MmAccessFault+332                                ; ----------------------------------------------------
//
//              Python> dump_bytes(0xFFFFF803FBE8E7C6, 0xFFFFF803FBE8E7F2-0xFFFFF803FBE8E7C6)
//
//              48 b8 00 d0 be 7d fb f6 ff ff 48 8b c0 4c 3b c8 0f 82 72 ff ff ff 48 b8 ff df be 7d fb f6 ff ff 48 8b c0 4c 3b c8 0f 87 5c ff ff ff

    PUCHAR    Routine = NULL;
    ULONG_PTR Return  = 0;

    USHORT PML4Instructions[] = { 0xff48, 0xffb8, 0x0000, 0x0000,
                                  0x0000, 0x0000, 0x0000, 0x0000,
                                  0x0000, 0x0000, 0xff48, 0xff8b,
                                  0xffc0, 0xff4c, 0xff3b, 0xffc8,
                                  0xff0f, 0xff82, 0xff72, 0xffff,
                                  0xffff, 0xffff, 0xff48, 0xffb8,
                                  0xffff, 0x0000, 0x0000, 0x0000,
                                  0x0000, 0x0000, 0x0000, 0x0000,
                                  0xff48, 0xff8b, 0xffc0, 0xff4c,
                                  0xff3b, 0xffc8, 0xff0f, 0xff87,
                                  0xff5c, 0xffff, 0xffff, 0xffff };

    UNICODE_STRING FunctionName = { 0 };

    RtlInitUnicodeString(&FunctionName, L"MmProbeAndLockPages");

    Routine = (PUCHAR)MmGetSystemRoutineAddress(&FunctionName);

    Routine += 0x3a1c;

    if (Routine) {
        ULONG Offset = MatchSignature( PML4Instructions, 
                                       sizeof(PML4Instructions) / sizeof(USHORT),
                                       Routine,
                                       0x500);

        if (Offset > 0) {
            Return = *(PULONG_PTR)&Routine[Offset + 51];

            // Add some verifications to retrieved offset
            // NT_ASSERT(Return & 0xFFFFFF0000000000 = 0xFFFFFF00000000);

            return Return;
        }
    }

    return Return;
}

//
//  Sanity check routine.
//
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

VOID
MmuFinalize(
    VOID
);

//#define MMU_LOWEST_USABLE_PHYSICAL_ADDRESS    (64 * 1024 * 1024)

NTSTATUS
MmuInitialize(
    VOID
    )
{
    LARGE_INTEGER MaxAddress = { MAXULONG64 };
    NTSTATUS Status = STATUS_INSUFFICIENT_RESOURCES;
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
    gMmu.PxeBase = (UINTN)MmuGetPxeAddress( 0 );

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

        gMmu.Processors[i].ZeroPte = MmuGetPxeAddress( ZeroPage );
        gMmu.Processors[i].ZeroPage = ZeroPage;

        for (j = 0; j < MAX_MAPPING_SLOTS; j++) {

            PteContents = *(PUINT64)gMmu.Processors[i].ZeroPte;
            Mapping = &gMmu.Processors[i].Mappings[j];

            Mapping->SystemVa = MmAllocateMappingAddress( PAGE_SIZE, 'MMAP' );

            if (NULL == Mapping->SystemVa) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto RoutineExit;
            }

            Mapping->PointerPte = MmuGetPxeAddress( Mapping->SystemVa );
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
                MmFreeMappingAddress( MmuVcpu->Mappings[j].SystemVa,
                                      'MMAP' );
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
MmuGetPxeAddress(
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
        start = (UINTN)MmuGetPxeAddress(0);
        end   = (UINTN)MmuGetPxeAddress(((PVOID)((UINTN)-1)));
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
