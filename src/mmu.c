#include "dsdef.h"
#include "mmu.h"
#include "smp.h"
#include "mem.h"
#include "os.h"

MMU gMmu = { 0 };

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

    if ( Routine ) {
        ULONG Offset = MatchSignature(PML4Instructions, sizeof(PML4Instructions) / sizeof(USHORT), Routine, 0x500);

        if ( Offset > 0 ) {

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

        for (i = 0; i < 512; i++)
        {
            UINT64 entry = pml4[i];

            if((entry & 0x7FFFFFFFFFFFF000) == cr3)
            {
                *autoEntryIndex = i;
                found = TRUE;
                break;
            }
        }

        MmUnmapIoSpace( cr3Va, PAGE_SIZE );
    }
   
    return found;
}

UINT64
MmupCreatePte(
    _In_ PHYSICAL_ADDRESS address,
    _In_ UINT64           flags
)
{
#ifdef _WIN64
    return (address.QuadPart & 0x0000FFFFFFFFF000) | flags;
#else
    if (gMmu.paeEnabled)
    {
        return (address.QuadPart & 0x0000FFFFFFFFF000) | flags;
    }
    else
    {
        return (address.LowPart & 0xFFFFF000) | (UINT32)flags;
    }
#endif
}

VOID
MmupChangeMapping(
    _In_ PMMU_MAPPING mapping,
    _In_ UINT64       pte
)
{
#ifdef _WIN64
    *(PUINT64)mapping->pte = pte;
#else
    if (gMmu.paeEnabled)
    {
        *(PUINT64)mapping->pte = pte;
    }
    else
    {
        *(PUINT32)mapping->pte = (UINT32)pte;
    }
#endif
    
    __invlpg(mapping->page);
}


PVOID
MmupMap(
    _In_ PHYSICAL_ADDRESS address,
    _In_ UINT64           flags
    )
{
    UINT32 i;
    PMMU_PROCESSOR MmuVcpu;

    MmuVcpu = &gMmu.Processors[SmpGetCurrentProcessor()];

    //
    // Find an empty slot in this Vcpu
    //
    for (i = 0; i < MAX_MAPPING_SLOTS; i++)
    {
        if (0 == InterlockedCompareExchange( &MmuVcpu->mappings[i].MapInUse, 1, 0 ) ) {

            //
            // Write adequate PTE in paging structures so 
            // physical address becomes accesible at this slot
            //
            MmupChangeMapping( &MmuVcpu->mappings[i],
                               MmupCreatePte( address, flags ));
            return MmuVcpu->mappings[i].page;
        }
    }

    return NULL;
}

VOID
MmuFinalize(
    VOID
);

NTSTATUS
MmuInitialize(
    VOID
    )
{
    UINT32 i, j;
#ifdef _WIN64
#if (NTDDI_VERSION >= NTDDI_VISTA)
    NTSTATUS Status;
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
        //  Find processor page structures in memory as now they are ASLR'd
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
        gMmu.paeEnabled = TRUE;
        gMmu.pxeShift = 3;
    }
    else
    {
        gMmu.paeEnabled = FALSE;
        gMmu.pxeShift = 2;
    }
#endif
    gMmu.pxeBase = (UINT_PTR)MmuGetPxeAddress(0);

    //
    //  Allocate one MMU per Vcpu.
    //
    gMmu.Processors = MemAllocArray(SmpActiveProcessorCount(), sizeof(MMU_PROCESSOR));
    
    if(!gMmu.Processors) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(gMmu.Processors, SmpActiveProcessorCount() * sizeof(MMU_PROCESSOR));

    for (i = 0; i < SmpActiveProcessorCount(); i++)
    {
        for (j = 0; j < MAX_MAPPING_SLOTS; j++)
        {
            //
            // Allocate mapping slot
            //
            PVOID slot = MmAllocateMappingAddress(4096, 'MMAP');
            if (slot == NULL)
                goto failure;

            gMmu.Processors[i].mappings[j].MapInUse = FALSE;
            gMmu.Processors[i].mappings[j].page     = slot;
            gMmu.Processors[i].mappings[j].pte      = MmuGetPxeAddress(slot);

            MmupChangeMapping(&gMmu.Processors[i].mappings[j], 0);
        }
    }

    return STATUS_SUCCESS;

failure:
    MmuFinalize();

    return STATUS_UNSUCCESSFUL;
}


VOID
MmuFinalize(
    VOID
    )
{
    UINT32 i, j;

    for (i = 0; i < SmpActiveProcessorCount(); i++)
    {
        for (j = 0; j < MAX_MAPPING_SLOTS; j++)
        {
            //
            // Free mapping slot
            //
            if(gMmu.Processors[i].mappings[j].page)
            {
                MmFreeMappingAddress(gMmu.Processors[i].mappings[j].page, 'MMAP');
            }
        }
    }

    MemFree(gMmu.Processors);
    gMmu.Processors = 0;

#ifdef _WIN64
    gMmu.lowerBound = 0;
    gMmu.upperBound = 0;
#else
    gMmu.paeEnabled = FALSE;
    gMmu.pxeShift = 0;
#endif
}


PVOID
MmuMap(
    _In_ PHYSICAL_ADDRESS address
    )
{
    //
    // Map with usual (present, etc) flags
    //
    return MmupMap(address, 0x03);
}

PVOID
MmuMapUncached(
    _In_ PHYSICAL_ADDRESS address
    )
{
    //
    // Map with no cache memory flags
    //
    return MmupMap(address, 0x1B);
}

VOID
MmuUnmap(
    _In_ PVOID address
    )
{
    UINT32 i;
    PMMU_PROCESSOR MmuVcpu;

    MmuVcpu = &gMmu.Processors[SmpGetCurrentProcessor()];
    
    //
    // Search address
    //
    for (i = 0; i < MAX_MAPPING_SLOTS; i++)
    {
        //
        // Entry is used and address match
        //
        if (MmuVcpu->mappings[i].MapInUse &&
            MmuVcpu->mappings[i].page == address) {

            InterlockedCompareExchange( &MmuVcpu->mappings[i].MapInUse, 0, 1 );

            //
            //  Set PTE to 0.
            //  TODO: is this arquitecturally valid (NXE)?.
            //

            MmupChangeMapping( &MmuVcpu->mappings[i], 0 );
            break;
        }
    }
}

PVOID
MmuGetPxeAddress(
    _In_opt_ PVOID address
    )
{
#ifdef _WIN64
    UINT64 result = 0;

    result = ((UINT64)address) >> 9;
    result = result | gMmu.lowerBound;
    result = result & gMmu.upperBound;

    return (PVOID)result;
#else
    UINT32 result = 0;

    result = ((UINT32)address) >> 12;
    result = result << gMmu.pxeShift;
    result = result + 0xC0000000;

    return (PVOID)result;
#endif
}

PVOID
MmuGetPageAddress(
    _In_ PVOID pxeAddress
)
{
    UINT_PTR pfn;
    UINT_PTR pageAddress;

#ifdef _WIN64
        pfn = ((UINT_PTR)pxeAddress - gMmu.pxeBase) / sizeof(UINT_PTR);

        pageAddress = pfn << 12;

        if (pageAddress & ((UINT64)1 << 47))
            pageAddress |= ((UINT64)0xFFFF << 48);
#else
    if(gMmu.paeEnabled)
    {
        pfn = ((UINT_PTR)pxeAddress - gMmu.pxeBase) / sizeof(UINT64);
    }
    else
    {
        pfn = ((UINT_PTR)pxeAddress - gMmu.pxeBase) / sizeof(UINT_PTR);
    }
    pageAddress = pfn << 12;
#endif

    return (PVOID)pageAddress;
}

BOOLEAN
MmuAddressIsInPagingRange(
    _In_ PVOID address
)
{
    static UINT_PTR start = 0;
    static UINT_PTR end   = 0;
    
    if(start == 0)
    {
        start = (UINT_PTR)MmuGetPxeAddress(0);
        end   = (UINT_PTR)MmuGetPxeAddress(((PVOID)((UINT_PTR)-1)));
    }

    return (start <= (UINT_PTR)address) && ((UINT_PTR)address <= end);
}



PHYSICAL_ADDRESS
MmuGetPhysicalAddress(
    _In_ UINT_PTR cr3,
    _In_ PVOID    address
)
{
    PHYSICAL_ADDRESS result;
    UINT_PTR         currentCr3;
    
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
            PVOID   pml;
            UINT16  index;
            UINT64  entry;

            pml = MmuMapUncached(pa);

            //
            // We went out of cached slots, exit
            //
            if ( !pml ) { 
                ASSERT( FALSE );
                break; 
            }

            index = MmuGetPxeIndex(level, address);
            entry = MmuGetPmlEntry(pml, index);
            MmuUnmap(pml);

            if(entry & 1)
            {
                if (entry & 8) {
                    //
                    //   Check what happens with a 2-MB page.
                    //
                    NT_ASSERT( FALSE );
                }

                if (level == 1)
                {
                    result.QuadPart = entry & 0xFFFFFFFFFFFFF000 | (((UINT_PTR)address) & 0xFFF);
                }
                else
                {
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
    if (gMmu.paeEnabled)
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
    if (gMmu.paeEnabled)
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
    if (gMmu.paeEnabled)
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
    sizeOfEntry = sizeof(UINT_PTR);
#else
    if (gMmu.paeEnabled)
    {
        sizeOfEntry = 8;
    }
    else
    {
        sizeOfEntry = sizeof(UINT_PTR);
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
    if (gMmu.paeEnabled)
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
    return  ((((UINT_PTR)va) >> (12 + 9 * (level - 1))) & 0x1FF);
#else
    if (gMmu.paeEnabled)
    {
        return  ((((UINT_PTR)va) >> (12 + 9 * (level - 1))) & 0x1FF);
    }
    else
    {
        return  ((((UINT_PTR)va) >> (12 + 10 * (level - 1))) & 0x3FF);
    }
#endif
}

BOOLEAN
MmuIsUserModeAddress(
    _In_ PVOID va
)
{
#ifdef _WIN64
    //return (((UINT_PTR)va) & 0x8000000000000000) == 0;
    return ((UINT_PTR)va) <= 0x00007FFFFFFFFFFF;
#else
    return  (((UINT_PTR)va) & 0x80000000) == 0;
#endif
}

BOOLEAN
MmuIsKernelModeAddress(
    _In_ PVOID va
)
{
#ifdef _WIN64
    //return (((UINT_PTR)va) & 0x8000000000000000) == 0x8000000000000000;
    return ((UINT_PTR)va) >= 0xFFFF800000000000;
#else
    return (((UINT_PTR)va) & 0x80000000) == 0x80000000;
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
