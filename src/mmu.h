/**
*  @file    mmu.h
*  @brief   Memory Management Unit functions
*/
#ifndef __MMU_H__
#define __MMU_H__

#include <ntifs.h>

#define CR4_PAE_ENABLED 0x20

#define MAX_MAPPING_SLOTS 4

typedef struct _MMU_MAPPING
{
    BOOLEAN  used;     //!< State oif the mapping
    PVOID    page;     //!< Virtual Address of the mapping where we can map memory
    PVOID    pte;      //!< Address of the pte to be able to map what we desire
} MMU_MAPPING, *PMMU_MAPPING;

typedef struct _MMU_CORE
{
    MMU_MAPPING mappings[MAX_MAPPING_SLOTS];
} MMU_CORE, *PMMU_CORE;

typedef struct _MMU
{
#ifdef _WIN64
    UINT64 autoEntryIndex;
    UINT64 lowerBound;
    UINT64 upperBound;
#else
    BOOLEAN paeEnabled;
    UINT32  pxeShift;
#endif
    UINT_PTR  pxeBase;
    PMMU_CORE cores;
} MMU, *PMMU;

/**
* Prepares and allocates resources for the logical MMU to work.
*
* @return Error if unable to complete the operation. Success otherwise.
*/
NTSTATUS
MmuInit(
    VOID
);

/**
* Releases all allocated resources related with the logical MMU.
*/
VOID
MmuDone(
    VOID
);

/**
* Will map a physical page to a virtual address.
* The mapping is not prepared to be permanent.
* Instead this operation allows to bring pages, perform
* modifications or reads on them and release (unmap).
*
* @param address [in] Physical address.
* @return Virtual address if mapped. NULL on error.
*/
PVOID
MmuMap(
    _In_ PHYSICAL_ADDRESS address
);

/**
* @see MmuMap.
* The mapping is set as "not cacheable".
* Useful for MMIO and hardware related pages.
*
* @param address [in] Physical address.
* @return Virtual address if mapped. NULL on error.
*/
PVOID
MmuMapUncached(
    _In_ PHYSICAL_ADDRESS address
);

/**
* Unmaps (and frees an underlying resource) a previously
* mapped page. Page should have been mapped with MmuMap 
* or MmuMapUncached.
*
* @param address [in] Previously returned virtual address.
*/
VOID
MmuUnmap(
    _In_ PVOID address
);

/**
* Obtains the physical address of a given virtual one.
* This translation is general.
*
* @param address [in] Virtual address.
* @return Physical address if there is translation. NULL on error or not existing address.
*/
PHYSICAL_ADDRESS
MmuGetPhysicalAddress(
    _In_ UINT_PTR cr3,
    _In_ PVOID    address
);


/**
* Obtains the virtual address of the PTE mapping a given VA.
*
* @param address [in] VA.
* @return Virtual address of the corresponding PTE mapping that VA.
*/
PVOID
MmuGetPxeAddress(
    _In_opt_ PVOID address
);

PVOID
MmuGetPageAddress(
    _In_ PVOID pxe
);

BOOLEAN
MmuAddressIsInPagingRange(
    _In_ PVOID address
);

UINT16
MmuGetNumberOfPageEntries(
    VOID
);

UINT8
MmuGetPageEntrySize(
    VOID
);

UINT64
MmuGetPmlEntry(
    _In_ PVOID  pml,
    _In_ UINT16 index
);

UINT16
MmuGetPmlIndexFromOffset(
    _In_ UINT32 offset
);

UINT8
MmuGetPagingLevels(
    VOID
);


UINT16
MmuGetPxeIndex(
    _In_ UINT8 level,
    _In_ PVOID va
);

BOOLEAN
MmuIsUserModeAddress(
    _In_ PVOID va
);

BOOLEAN
MmuIsKernelModeAddress(
    _In_ PVOID va
);

BOOLEAN
MmuIsInvalidAddress(
    _In_ PVOID va
);

#endif
