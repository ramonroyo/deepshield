/**
*  @file    mmu.h
*  @brief   Memory Management Unit functions
*/
#ifndef __MMU_H__
#define __MMU_H__

#include <ntifs.h>

#pragma warning(disable:4201)   // nameless struct/union
#pragma warning(disable:4214)   // bit field types other than int

#define CR4_PAE_ENABLED 0x20

#define MAX_MAPPING_SLOTS 4

typedef struct _MMU_MAPPING
{
    volatile LONG MapInUse;     //!< State of the mapping.
    PVOID SystemVa;             //!< Virtual Address of the mapping where we can map memory.
    PVOID PointerPte;                //!< Address of the PTE to be able to map on demand.
} MMU_MAPPING, *PMMU_MAPPING;

typedef struct _MMU_PROCESSOR
{
    PVOID ZeroPage;
    PVOID ZeroPte;
    MMU_MAPPING Mappings[MAX_MAPPING_SLOTS];
} MMU_PROCESSOR, *PMMU_PROCESSOR;

typedef struct _MMU
{
#ifdef _WIN64
    UINT64 autoEntryIndex;
    UINT64 lowerBound;
    UINT64 upperBound;
#else
    BOOLEAN PaeEnabled;
    UINT32  PxeShift;
#endif
    UINTN  PxeBase;
    PMMU_PROCESSOR Processors;
} MMU, *PMMU;

//
//  Define bit fields for PTE and PDE entry.
//

#define PTE_PRESENT       1ULL
#define PTE_RW            (1ULL << 1)
#define PTE_USER          (1ULL << 2)
#define PTE_PWT           (1ULL << 3)
#define PTE_PCD           (1ULL << 4)
#define PTE_ACCESSED      (1ULL << 5)
#define PTE_DIRTY         (1ULL << 6)
#define PTE_PAT           (1ULL << 7)
#define PTE_GLOBAL        (1ULL << 8)
#define PTE_XD            (1ULL << 63)

#define PTE_PRESENT       1ULL
#define PDE_RW            (1ULL << 1)
#define PDE_USER          (1ULL << 2)
#define PDE_PWT           (1ULL << 3)
#define PDE_PCD           (1ULL << 4)
#define PDE_ACCESSED      (1ULL << 5)
#define PDE_DIRTY         (1ULL << 6)    // only valid with PSE=1
#define PDE_PSE           (1ULL << 7)
#define PDE_GLOBAL        (1ULL << 8)    // only valid with PSE=1
#define PDE_PAT_LARGE     (1ULL << 12)   // only valid with PSE=1
#define PDE_XD            (1ULL << 63)

#define PAGE_CACHE_MASK      (PTE_PCD | PTE_PWT)
#define PAGE_CACHE_WB        0ULL
#define PAGE_CACHE_WT        PTE_PWT
#define PAGE_CACHE_UC_MINUS  PTE_PCD
#define PAGE_CACHE_UC        (PTE_PCD | PTE_PWT)

#define PAGE_TABLE           (PTE_PRESENT | PTE_RW | PTE_USER)

typedef struct PTE_ENTRY32
{
    union
    {
        struct
        {
            UINT32 Present : 1;
            UINT32 Writable : 1;
            UINT32 UserPage : 1;
            UINT32 WriteThrough : 1;
            UINT32 CacheDisable : 1;
            UINT32 Accessed : 1;
            UINT32 x1 : 2;
            UINT32 GlobalPage : 1;
            UINT32 x2 : 23;
        } Bits;

        struct
        {
            UINT32 x3 : 6;            // The 6 bits are always identical
            UINT32 Dirty : 1;
            UINT32 Pat : 1;
            UINT32 x4 : 4;
            UINT32 BaseAddress : 20;
        } Pte;

        struct
        {
            UINT32 x5 : 7;            // The 7 bits are always identical
            UINT32 LargePage : 1;
            UINT32 x6 : 4;
            UINT32 BaseAddress : 20;
        } Pde;

        struct
        {
            UINT32 x7 : 7;            // The 7 bits are always identical
            UINT32 LargePage : 1;
            UINT32 x8 : 4;
            UINT32 Pat : 1;
            UINT32 x9 : 9;
            UINT32 BaseAddress : 10;
        } Pde4M;

        UINT32 AsUintN;
    };
} PTE_ENTRY32;

typedef struct _PTE_ENTRY64
{
    union
    {
        struct
        {
            UINT64 Present : 1;
            UINT64 Writable : 1;
            UINT64 UserPage : 1;
            UINT64 WriteThrough : 1;
            UINT64 CacheDisable : 1;
            UINT64 Accessed : 1;
            UINT64 Available : 2;
            UINT64 GlobalPage : 1;
            UINT64 Lock : 1;
            UINT64 x1 : 2;
            UINT64 BaseAddress : 28;
            UINT64 Rsvd : 23;
            UINT64 NoExecute : 1;
        } Bits;
        
        struct
        {
            UINT64 x2 : 6;            // The 6 bits are always identical
            UINT64 Dirty : 1;
            UINT64 Pat : 1;
            UINT64 x3 : 4;
            UINT64 BaseAddress : 28;
            UINT64 x4 : 24;
        } Pte;
        
        struct
        {
            UINT64 x5 : 7;            // The bits are identical or reserved
            UINT64 LargePage : 1;     // Page size
            UINT64 x6 : 3;
            UINT64 Pat : 1;
            UINT64 BaseAddress : 28;
            UINT64 x7 : 24;
        } Pde;

        UINT64 AsUintN;
    };
} PTE_ENTRY64;

#ifdef _WIN64
typedef PTE_ENTRY64 PTE_ENTRY;
#else
typedef PTE_ENTRY32 PTE_ENTRY;
#endif

/**
* Prepares and allocates resources for the logical MMU to work.
*
* @return Error if unable to complete the operation. Success otherwise.
*/
NTSTATUS
MmuInitialize(
    VOID
);

/**
* Releases all allocated resources related with the logical MMU.
*/
VOID
MmuFinalize(
    VOID
);

/**
* Will map a physical page to a virtual address.
* The mapping is not prepared to be permanent.
* Instead this operation allows to bring pages, perform
* modifications or reads on them and release (unmap).
* Opyionally the mapping is set as "not cacheable".
* Useful for MMIO and hardware related pages
*
* @param address [in] Physical address.
* @return Virtual address if mapped. NULL on error.
*/
PVOID
MmuMapPage(
    _In_ PHYSICAL_ADDRESS PhysicalPage,
    _In_ BOOLEAN Uncached
);

/**
* Unmaps (and frees an underlying resource) a previously
* mapped page. Page should have been mapped with MmuMapPage
*
* @param address [in] Previously returned virtual address.
*/
VOID
MmuUnmapPage(
    _In_ PVOID VirtualAddress
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
    _In_ UINTN Cr3,
    _In_ PVOID Address
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
MmuGetPhysicalAddressMappedByPte(
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
