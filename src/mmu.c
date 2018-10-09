#include "dsdef.h"
#include "mmu.h"
#include "smp.h"
#include "mem.h"
#include "os.h"


static CHAR MappingSpace[MAX_CPU + 1][MAX_MAPPING_SLOTS][PAGE_SIZE] = { 0 };

MMU gMmu = { 0 };

//
//  Template PTE for a kernel page.
//
UINT64 DefaultPte = PTE_VALID | PTE_RW | PTE_DIRTY | PTE_ACCESSED;
UINT64 ZeroPte = 0ULL;

#ifdef _WIN64

_Success_(return)
BOOLEAN
MmuGetSelfMapPxeIndex(
    _Out_ PUINT64 SelfMapPxeIndex
    )
{
    PHYSICAL_ADDRESS Cr3;
    Cr3.QuadPart = __readcr3() & 0xFFFFFFFFFFFFF000;

    //
    //  The PML4 page is always mapped in the self referenced PXE index.
    //
    *SelfMapPxeIndex = MmuGetPxeIndex( MmGetVirtualForPhysical( Cr3 ) );
    NT_ASSERT( *SelfMapPxeIndex >= 16 );

    return TRUE;
}
#endif

UINT64
MmupCreatePte(
    _In_ PHYSICAL_ADDRESS PhysicalAddress,
    _In_ UINT64 PteContents
    )
{
#ifdef _WIN64
    return (PhysicalAddress.QuadPart & PTE64_PFN_MASK) | PteContents;

#else
    if (gMmu.PaeEnabled) {
        return (PhysicalAddress.QuadPart & PTE64_PFN_MASK) | PteContents;
    }
    else {
        return (PhysicalAddress.LowPart & PTE32_PFN_MASK) | (UINT32)PteContents;
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
    __invlpg( Mapping->BaseVa );
}

PVOID
MmuAddressToPte(
    _In_ PVOID Address
    )
{
    UINTN PointerPte;

    //
    //  MmuAddressToPte returns the address of the PTE which maps the given
    //  virtual address.
    //

#ifdef _WIN64
    PointerPte = ((UINT64)Address) >> (PTI_SHIFT - 3);
    PointerPte |= gMmu.LowerBound;
    PointerPte &= gMmu.UpperBound;
#else
    PointerPte = ((UINT32)Address) >> 12;
    PointerPte = PointerPte << gMmu.PxeShift;
    PointerPte = PointerPte + 0xC0000000;
#endif

    return (PVOID)PointerPte;
}

#if (NTDDI_VERSION >= NTDDI_VISTA) && defined(_WIN64)
PVOID
MmuPteToAddress(
    _In_ PPTE PointerPte
    )
{
    //
    //  Transform a PTE into its corresponding virtual address.
    //
    return (PVOID) (((INT64)PointerPte << 25) >> 16);
}

PPTE
MmuFindPteForMdl(
    _In_ PMDL BaseMdl
    )
{
    NTSTATUS Status;
    PFN_NUMBER PageFrameIndex;
    PMMPFN MmPfnDatabase;
    PMMPFN PfnEntry;

    Status = OsGetPfnDatabase( (PUINT64)&MmPfnDatabase );

    if (!NT_SUCCESS( Status )) {
        return NULL;
    }

    PageFrameIndex = MmGetMdlPfnArray( BaseMdl )[0];

    //
    //  Sanity check for the page frame number.
    //
    NT_ASSERT( (PageFrameIndex << PAGE_SHIFT) == (UINT64)
              MmGetPhysicalAddress( MmGetMdlBaseVa( BaseMdl ) ).QuadPart );

    //
    //  Note that the PFN points at a 'hardware' PTE.
    //
    PfnEntry = &MmPfnDatabase[PageFrameIndex];

    //
    //  Sanity checks for the returned PTE.
    //
    NT_ASSERT( MmGetMdlBaseVa( BaseMdl ) == 
                                 MmuPteToAddress( PfnEntry->PteAddress ) );
    return PfnEntry->PteAddress;
}

PVOID
MmuGetPteBase(
    VOID
    )
{
    PVOID ProbeVa = NULL;
    PMDL ProbeMdl = NULL;
    UINT64 PteBase = 0;
    UINT64 PointerPte;
    UINT64 PartialPte;

    //
    //  Allocate the page-aligned probe buffer.
    //
    
    ProbeVa = ExAllocatePoolWithTag( NonPagedPool, PAGE_SIZE, 'bPsD' );
    if (ProbeVa == NULL) {
        goto RoutineExit;
    }

    ProbeMdl = IoAllocateMdl( ProbeVa, PAGE_SIZE, FALSE, FALSE, NULL );
    if (ProbeMdl == NULL) {
        goto RoutineExit;
    }
    
    //
    //  Update the MDL to describe the underlying physical pages.
    //
    MmBuildMdlForNonPagedPool( ProbeMdl );

    //
    //  Find the PTE controlling the frame described by the MDL.
    //
    PointerPte = (UINT64)MmuFindPteForMdl( ProbeMdl );
    
    if (PointerPte) {
        //
        //  Get the base subtracting the partial PTE for the probe address.
        //
        PartialPte = ((((UINT64) ProbeVa & VA_MASK) >> PTI_SHIFT) << PTE_SHIFT);
        PteBase = PointerPte - PartialPte;
    }

RoutineExit:

    if (ProbeVa) {
        ExFreePoolWithTag( ProbeVa, 'bPsD' );

        if (ProbeMdl) {
            IoFreeMdl( ProbeMdl );
        }
    }

    return (PVOID)PteBase;
}
#endif

PMDL
MmuAllocateMappingMdl(
    _In_ UINT32 Index,
    _In_ UINT32 Slot
    ) 
{
    PMDL MappingMdl;
    PVOID SlotSpace;

    if (Index > MAX_CPU) {
        return NULL;
    }

    //
    //  Use data section as non-paged pool pages allocated from the OS large
    //  page cache are not suitable for PTE remapping.
    //
    SlotSpace = (PVOID)ROUND_TO_PAGES( &MappingSpace[Index][Slot][0] );

    MappingMdl = IoAllocateMdl( SlotSpace, PAGE_SIZE, FALSE, FALSE, NULL );

    if (NULL == MappingMdl ) {
		return NULL;
	}

	try {
		//
        //  Makes resident and lock the physical page.
        //
		MmProbeAndLockPages( MappingMdl, KernelMode, IoWriteAccess );

	} except( EXCEPTION_EXECUTE_HANDLER ) {

		IoFreeMdl( MappingMdl );
	}

    return MappingMdl;
}

VOID
MmuFreeMappingMdl(
    _Inout_ PMDL MappingMdl
    )
{
    NT_ASSERT( MappingMdl );

    MmUnlockPages( MappingMdl );
    IoFreeMdl( MappingMdl );
}

NTSTATUS
MmuInitializeMapping(
    _Inout_ PMMU_MAPPING Mapping,
    _In_ UINT32 ProcIdx,
    _In_ UINT32 SlotIdx
    )
{
    Mapping->BaseMdl = MmuAllocateMappingMdl( ProcIdx, SlotIdx );

    if (NULL == Mapping->BaseMdl) {
        return STATUS_NO_MEMORY;
    }

    Mapping->BaseVa = MmMapLockedPagesSpecifyCache( Mapping->BaseMdl,
                                                    KernelMode,
                                                    MmCached,
                                                    NULL,
                                                    0,
                                                    NormalPagePriority );

    if (NULL == Mapping->BaseVa) {
        MmuFreeMappingMdl( Mapping->BaseMdl );
        Mapping->BaseMdl = NULL;

        return STATUS_NO_MEMORY;
    }

    //
    //  The virtual address is locked so we don't need to traverses the page
    //  tables to find the corresponding pte.
    //

    Mapping->PointerPte = MmuAddressToPte( Mapping->BaseVa );
    Mapping->OriginalPte = *(PUINT64) Mapping->PointerPte;
    
    MmupWriteMappingPte( Mapping, ZeroPte );
    Mapping->MapInUse = FALSE;

    return STATUS_SUCCESS;
}

VOID
MmuDeleteMapping(
    _Inout_ PMMU_MAPPING Mapping
    )
{
    NT_ASSERT( Mapping->BaseVa );

    //
    //  Restore PTE content before unlocking memory.
    //

    MmupWriteMappingPte( Mapping, Mapping->OriginalPte );

    MmUnmapLockedPages( Mapping->BaseVa, Mapping->BaseMdl );
    MmuFreeMappingMdl( Mapping->BaseMdl );
}

#if !defined (_WIN64)
BOOLEAN
MmuIsPaeEnabled(
    VOID
    )
{
    ULONG_PTR cr4 = __readcr4();

    if (cr4 & CR4_PAE_ENABLED) {
        return TRUE;
    }

    return FALSE;
}
#endif

#if defined (_WIN64)
NTSTATUS
MmuInitializeMmuGlobals(
    VOID
    )
{
    //
    //  Locate the PTE entries in memory.
    //
    if (!MmuGetSelfMapPxeIndex( &gMmu.SelfMapPxeIndex )) {
        return STATUS_UNSUCCESSFUL;
    }

    gMmu.LowerBound = gMmu.SelfMapPxeIndex << PXI_SHIFT | 0xFFFFULL << VA_BITS;

#if DBG && (NTDDI_VERSION >= NTDDI_VISTA)
    NT_ASSERT( gMmu.LowerBound == (UINT64)MmuGetPteBase() );
#endif
    
    //
    //  The limit is 512GB (512*512*512*4096) away.
    //
    gMmu.UpperBound = (gMmu.LowerBound + 0x8000000000 - 1) & 0xFFFFFFFFFFFFFFF8;
    gMmu.PteBase = (UINTN)MmuAddressToPte( 0 );

    return STATUS_SUCCESS;
}
#else
NTSTATUS
MmuInitializeMmuGlobals(
    VOID
    )
{
    gMmu.PaeEnabled = MmuIsPaeEnabled();

    if (gMmu.PaeEnabled) {
        gMmu.PxeShift = 3;

    } else {
        gMmu.PxeShift = 2;
    }

    gMmu.PteBase = (UINTN)MmuAddressToPte( 0 );
    return STATUS_SUCCESS;
}

#endif

NTSTATUS
MmuInitialize(
    VOID
    )
{
    NTSTATUS Status = STATUS_NO_MEMORY;
    PMMU_PERCPU Mmu;
    UINT32 ProcessorCount = SmpActiveProcessorCount();
    UINT32 ProcIdx;
    UINT32 SlotIdx;

    Status = MmuInitializeMmuGlobals();
    if (!NT_SUCCESS( Status )) {
        return Status;
    }

    gMmu.MmuArray = MemAllocArray( ProcessorCount, sizeof( MMU_PERCPU ));
    if (!gMmu.MmuArray) {
        return Status;
    }

    RtlZeroMemory( gMmu.MmuArray, ProcessorCount * sizeof( MMU_PERCPU ));

    for (ProcIdx = 0; ProcIdx < ProcessorCount; ProcIdx++) {
        Mmu = &gMmu.MmuArray[ProcIdx];

        for (SlotIdx = 0; SlotIdx < MAX_MAPPING_SLOTS; SlotIdx++) {
            Status = MmuInitializeMapping( &Mmu->Mappings[SlotIdx],
                                           ProcIdx,
                                           SlotIdx );

            if (!NT_SUCCESS( Status )) {
                goto RoutineExit;
            }
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
    PMMU_PERCPU Mmu;
    UINT32 ProcIdx;
    UINT32 SlotIdx;

    for (ProcIdx = 0; ProcIdx < SmpActiveProcessorCount(); ProcIdx++) {
        Mmu = &gMmu.MmuArray[ProcIdx];

        for (SlotIdx = 0; SlotIdx < MAX_MAPPING_SLOTS; SlotIdx++) {

            //
            //  Free all the PTE mappings.
            //
            if (Mmu->Mappings[SlotIdx].BaseVa) {
                MmuDeleteMapping( &Mmu->Mappings[SlotIdx] );
            }
        }
    }

    MemFree( gMmu.MmuArray );
    gMmu.MmuArray = NULL;

#ifdef _WIN64
    gMmu.LowerBound = 0;
    gMmu.UpperBound = 0;
#else
    gMmu.PaeEnabled = FALSE;
    gMmu.PxeShift = 0;
#endif
}

BOOLEAN
MmuAcquireMapping(
    _Deref_out_ PMMU_MAPPING *Mapping
    )
{
    PMMU_PERCPU Mmu;
    UINT32 SlotIdx;

    Mmu = &gMmu.MmuArray[ SmpGetCurrentProcessor() ];

    for (SlotIdx = 0; SlotIdx < MAX_MAPPING_SLOTS; SlotIdx++) {
        if (InterlockedCompareExchange( &Mmu->Mappings[SlotIdx].MapInUse,
                                        1,
                                        0 ) == 0) {

            *Mapping = &Mmu->Mappings[SlotIdx];
            return TRUE;
        }
    }

    return FALSE;
}

PMMU_MAPPING
MmuGetMappingForVa(
    _In_ PVOID VirtualAddress
    )
{
    PMMU_PERCPU Mmu;
    UINT32 SlotIdx;

    Mmu = &gMmu.MmuArray[ SmpGetCurrentProcessor() ];

    for (SlotIdx = 0; SlotIdx < MAX_MAPPING_SLOTS; SlotIdx++ ) {

        if (Mmu->Mappings[SlotIdx].MapInUse &&
            Mmu->Mappings[SlotIdx].BaseVa == VirtualAddress) {

            return &Mmu->Mappings[SlotIdx];
        }
    }

    return NULL;
}

VOID
MmuReleaseMapping(
    _Inout_ PMMU_MAPPING Mapping
    )
{
    InterlockedCompareExchange( &Mapping->MapInUse, 0, 1 );
}

PVOID
MmuMapIoPage(
    _In_ PHYSICAL_ADDRESS PhysicalPage,
    _In_ BOOLEAN Uncached
    )
{
    PMMU_MAPPING Mapping;
    UINT64 TempPte = DefaultPte;

    if (MmuAcquireMapping( &Mapping )) {

        if (Uncached) {
            TempPte |= PAGE_CACHE_UC;
        }

        TempPte = MmupCreatePte( PhysicalPage, TempPte );
        MmupWriteMappingPte( Mapping, TempPte );

        return Mapping->BaseVa;
    }

    return NULL;
}

VOID
MmuUnmapIoPage(
    _In_ PVOID VirtualAddress
    )
{
    PMMU_MAPPING Mapping = MmuGetMappingForVa( VirtualAddress );

    if (Mapping) {
        MmupWriteMappingPte( Mapping, ZeroPte );
        MmuReleaseMapping( Mapping );
    }
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
        start = (UINTN)MmuAddressToPte(0);
        end   = (UINTN)MmuAddressToPte(((PVOID)((UINTN)-1)));
    }

    return (start <= (UINTN)address) && ((UINTN)address <= end);
}

PHYSICAL_ADDRESS
MmuGetPhysicalAddress(
    _In_ UINTN TargetCr3,
    _In_ PVOID BaseAddress
    )
{
    UINTN CurrentCr3;
    UINT8 PagingLevel = MmuGetPagingLevels();
    PHYSICAL_ADDRESS ReturnPa = { 0 };
    PHYSICAL_ADDRESS CurrentPa = { 0 };
    PVOID PageTable;
    UINT64 PteEntry;
    UINT16 PteIndex;
    
    TargetCr3 &= 0xFFFFFFFFFFFFF000;
    CurrentCr3 = __readcr3() & 0xFFFFFFFFFFFFF000;

    if (TargetCr3 == 0 || TargetCr3 == CurrentCr3 ) {
        //
        //  Mapped page tables correspond to this process.
        //

        ReturnPa = MmGetPhysicalAddress( BaseAddress );
        return ReturnPa;
    }

    CurrentPa.QuadPart = TargetCr3;

    while (PagingLevel) {

        PageTable = MmuMapIoPage( CurrentPa, TRUE );
        if (!PageTable ) {

            //
            //  No more mapping slots.
            //
            NT_ASSERT( FALSE );
            break; 
        }

        PteIndex = MmuAddressToPti( PagingLevel, BaseAddress );
        PteEntry = MmuGetPmlEntry( PageTable, PteIndex );

        MmuUnmapIoPage( PageTable );

        if (0 == (PteEntry & PTE_VALID)) {
            break;
        }

        if (PagingLevel != 1 && (PteEntry & PDE_PSE)) {
            //
            //   Fail if PDE is for a 2-MB page.
            //
            NT_ASSERT( FALSE );
            break;
        }

        if (PagingLevel == 1) {
            ReturnPa.QuadPart = 
                PteEntry & 0xFFFFFFFFFFFFF000 | (((UINTN)BaseAddress) & 0xFFF);
        }
        else {
            CurrentPa.QuadPart = PteEntry & 0xFFFFFFFFFFFFF000;
        }

        --PagingLevel;
    }

    return ReturnPa;
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

#ifdef _WIN64
UINT64
MmuGetPmlEntry(
    _In_ PVOID Pml,
    _In_ UINT16 Index
    )
{
    PUINT64 PageTable = (PUINT64) Pml;
    return PageTable[Index];
}
#else
UINT64
MmuGetPmlEntry(
    _In_ PVOID Pml,
    _In_ UINT16 Index
    )
{
    if (gMmu.PaeEnabled) {
        PUINT64 PageTable = (PUINT64) Pml;
        return PageTable[Index];
    }
    else {
        PUINT32 PageTable = (PUINT32) Pml;
        return PageTable[ Index ];
    }
}
#endif

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
MmuAddressToPti(
    _In_ UINT8 Level,
    _In_ PVOID Address
    )
{
#ifdef _WIN64
    return  ((((UINT64)Address) >> (PTI_SHIFT + 9 * (Level - 1))) & 0x1FF);
#else
    if (gMmu.PaeEnabled) {
        return  ((((UINT32)Address) >> (PTI_SHIFT + 9 * (Level - 1))) & 0x1FF);
    }
    else {
        return  ((((UINT32)Address) >> (PTI_SHIFT + 10 * (Level - 1))) & 0x3FF);
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
