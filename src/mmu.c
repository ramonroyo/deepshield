#include <ntddk.h>
#include "dsdef.h"
#include "mmu.h"
#include "smp.h"
#include "mem.h"

MMU gMmu = { 0 };

_Success_(return)
BOOLEAN
MmupLocateAutoEntryIndex(
    _Out_ PUINT64 autoEntryIndex
)
{
    UINT64           cr3   = 0;
    PHYSICAL_ADDRESS cr3Pa = { 0 };
    PVOID            cr3Va = 0;
    BOOLEAN          found = FALSE;
    
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

        MmUnmapIoSpace(cr3Va, PAGE_SIZE);
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
    UINT32 core;

    core = SmpCurrentCore();

    //
    // Find an empty slot in this core
    //
    for (i = 0; i < MAX_MAPPING_SLOTS; i++)
    {
        if (!gMmu.cores[core].mappings[i].used)
        {
            gMmu.cores[core].mappings[i].used = TRUE;

            //
            // Write adequate PTE in paging structures so 
            // physical address becomes accesible at this slot
            //
            MmupChangeMapping(&gMmu.cores[core].mappings[i], MmupCreatePte(address, flags));

            return gMmu.cores[core].mappings[i].page;
        }
    }

    return 0;
}

VOID
MmuDone(
    VOID
);

NTSTATUS
MmuInit(
    VOID
)
{
    UINT32   i, j;

#ifdef _WIN64
    //
    // Find processor page structures in memory as now they are ASLR'd
    //
    if(!MmupLocateAutoEntryIndex(&gMmu.autoEntryIndex))
        return STATUS_UNSUCCESSFUL;

    gMmu.lowerBound =  (((UINT64)0xFFFF) << 48) | (gMmu.autoEntryIndex << 39);
    gMmu.upperBound = ((((UINT64)0xFFFF) << 48) | (gMmu.autoEntryIndex << 39) + 0x8000000000 - 1) & 0xFFFFFFFFFFFFFFF8;
#else
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
    // Allocate one MMU per core
    //
    gMmu.cores = MemAllocArray(SmpNumberOfCores(), sizeof(MMU_CORE));
    if(!gMmu.cores)
        return STATUS_INSUFFICIENT_RESOURCES;

    memset(gMmu.cores, 0, SmpNumberOfCores() * sizeof(MMU_CORE));

    for (i = 0; i < SmpNumberOfCores(); i++)
    {
        for (j = 0; j < MAX_MAPPING_SLOTS; j++)
        {
            //
            // Allocate mapping slot
            //
            PVOID slot = MmAllocateMappingAddress(4096, 'MMAP');
            if (slot == NULL)
                goto failure;

            //
            // Initialize
            //
            gMmu.cores[i].mappings[j].used     = FALSE;
            gMmu.cores[i].mappings[j].page     = slot;
            gMmu.cores[i].mappings[j].pte      = MmuGetPxeAddress(slot);

            MmupChangeMapping(&gMmu.cores[i].mappings[j], 0);
        }
    }

    return STATUS_SUCCESS;

failure:
    MmuDone();

    return STATUS_UNSUCCESSFUL;
}


VOID
MmuDone(
    VOID
)
{
    UINT32 i, j;

    for (i = 0; i < SmpNumberOfCores(); i++)
    {
        for (j = 0; j < MAX_MAPPING_SLOTS; j++)
        {
            //
            // Free mapping slot
            //
            if(gMmu.cores[i].mappings[j].page)
            {
                MmFreeMappingAddress(gMmu.cores[i].mappings[j].page, 'MMAP');
            }
        }
    }

    MemFree(gMmu.cores);
    gMmu.cores = 0;

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
    UINT32 core;

    core = SmpCurrentCore();
    
    //
    // Search address
    //
    for (i = 0; i < MAX_MAPPING_SLOTS; i++)
    {
        //
        // Entry is used and address match
        //
        if (gMmu.cores[core].mappings[i].used && 
            gMmu.cores[core].mappings[i].page == address)
        {
            gMmu.cores[core].mappings[i].used = FALSE;

            //
            // Set PTE to 0
            //
            MmupChangeMapping(&gMmu.cores[core].mappings[i], 0);

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

    if(cr3 == 0 || cr3 == currentCr3)
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
        UINT8            level = MmuGetPagingLevels();

        result.QuadPart = 0;
        pa.QuadPart = cr3;

        while(level)
        {
            PVOID   pml;
            UINT16  index;
            UINT64  entry;

            pml   = MmuMapUncached(pa);
            index = MmuGetPxeIndex(level, address);
            entry = MmuGetPmlEntry(pml, index);
            MmuUnmap(pml);

            if(entry & 1)
            {
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
