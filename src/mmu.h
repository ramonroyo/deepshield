/**
*  @file    mmu.h
*  @brief   Memory Management Unit functions
*/
#ifndef __MMU_H__
#define __MMU_H__

#include <ntifs.h>

#pragma warning(disable:4201)   // nameless struct/union
#pragma warning(disable:4214)   // bit field types other than int

#define MAX_MAPPING_SLOTS 4
#define MAX_CPU 64

#define PAGE_MASK (~(PAGE_SIZE - 1))
#define PAGE_ALIGN(va) ((PVOID)((ULONG_PTR)(va) & ~(PAGE_SIZE - 1)))

#define PFN_TO_PAGE(pfn) (pfn << PAGE_SHIFT)
#define PAGE_TO_PFN(pfn) (pfn >> PAGE_SHIFT)

#define PTE32_PFN_MASK   0xFFFFF000
#define PTE64_PFN_MASK   0x000FFFFFFFFFF000

#define MmuGetPfnFromPte(PTE) ((PTE)->Pte.PageFrame)

#if defined(_WIN64)

#define VA_BITS       48
#define VA_BITS_LA57  57

#define PZI_BITS    9
#define PXI_BITS    9
#define PPI_BITS    9
#define PDI_BITS    9
#define PTI_BITS    9

#ifndef PZI_MASK
#define PZI_MASK      ((1 << PZI_BITS) - 1)
#endif

#ifndef PXI_MASK
#define PXI_MASK      ((1 << PXI_BITS) - 1)
#endif

#ifndef PPI_MASK
#define PPI_MASK      ((1 << PPI_BITS) - 1)
#endif

#ifndef PDI_MASK
#define PDI_MASK      ((1 << PDI_BITS) - 1)
#endif

#ifndef PTI_MASK
#define PTI_MASK      ((1 << PTI_BITS) - 1)
#endif

#define VA_MASK       ((1ULL << VA_BITS) - 1)
#define VA_MASK_LA57  ((1ULL << VA_BITS_LA57) - 1)

#ifndef PZI_SHIFT
#define PZI_SHIFT 48
#endif

#ifndef PXI_SHIFT
#define PXI_SHIFT 39
#endif

#ifndef PPI_SHIFT
#define PPI_SHIFT 30
#endif

#ifndef PDI_SHIFT
#define PDI_SHIFT 21
#endif

#ifndef PTI_SHIFT
#define PTI_SHIFT 12
#endif

#ifndef PTE_SHIFT
#define PTE_SHIFT 3
#endif

//
//  Define the macros for those paging level supported in MmuAddressToPti
//  routine.
//
#define MmuGetPzeIndex(va) ((((UINT64)(va)) >> PZI_SHIFT) & PZI_MASK)
#define MmuGetPxeIndex(va) ((((UINT64)(va)) >> PXI_SHIFT) & PXI_MASK)
#define MmuGetPpeIndex(va) ((((UINT64)(va)) >> PPI_SHIFT) & PPI_MASK)
#define MmuGetPdeIndex(va) ((((UINT64)(va)) >> PDI_SHIFT) & PDI_MASK)
#define MmuGetPteIndex(va) ((((UINT64)(va)) >> PTI_SHIFT) & PTI_MASK)

#endif

typedef struct _MMU_MAPPING
{
    volatile LONG MapInUse;          //!< State of the mapping.
    PVOID BaseVa;                    //!< Virtual Address of the mapping where we can map memory.
    PVOID PointerPte;                //!< Address of the PTE to be able to map on demand.
    PMDL BaseMdl;
    UINT64 OriginalPte;
} MMU_MAPPING, *PMMU_MAPPING;

typedef struct _MMU_PERCPU
{
    MMU_MAPPING Mappings[MAX_MAPPING_SLOTS];
} MMU_PERCPU, *PMMU_PERCPU;

typedef struct _MMU
{
#ifdef _WIN64
    UINT64 SelfMapPmlIndex;
    UINT64 LowerBound;
    UINT64 UpperBound;
    BOOLEAN La57Enabled;
#else
    BOOLEAN PaeEnabled;
    UINT32 PxeShift;
#endif
    UINTN PteBase;
    PMMU_PERCPU MmuArray;
} MMU, *PMMU;

//
//  Define bit fields for PTE and PDE entry.
//

#define PTE_VALID         1ULL
#define PTE_RW            (1ULL << 1)
#define PTE_USER          (1ULL << 2)
#define PTE_PWT           (1ULL << 3)
#define PTE_PCD           (1ULL << 4)
#define PTE_ACCESSED      (1ULL << 5)
#define PTE_DIRTY         (1ULL << 6)
#define PTE_PAT           (1ULL << 7)
#define PTE_GLOBAL        (1ULL << 8)
#define PTE_XD            (1ULL << 63)

#define PTE_VALID         1ULL
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
            UINT32 x3 : 6;               // The 6 bits are always identical
            UINT32 Dirty : 1;
            UINT32 Pat : 1;
            UINT32 x4 : 4;
            UINT32 PageFrame : 20;
        } Pte;

        struct
        {
            UINT32 x5 : 7;               // The 7 bits are always identical
            UINT32 LargePage : 1;
            UINT32 x6 : 4;
            UINT32 PageFrame : 20;
        } Pde;

        struct
        {
            UINT32 x7 : 6;               // The 6 bits are always identical
            UINT32 Dirty : 1;
            UINT32 LargePage : 1;
            UINT32 x8 : 4;
            UINT32 Pat : 1;
            UINT32 PageFrameTo39 : 4;    //  When MAXPHYADDR is 40 (PSE-36)
            UINT32 x9 : 5;
            UINT32 PageFrame : 10;
        } Pde4M;                         //  When CR4.PSE = 1.

        UINT32 AsUint32;
    };
} PTE32;

typedef struct _PTE64
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
            UINT64 PageFrame : 40;
            UINT64 Rsvd : 11;
            UINT64 NoExecute : 1;
        } Bits;
        
        struct
        {
            UINT64 x2 : 6;            // The 6 bits are always identical
            UINT64 Dirty : 1;
            UINT64 Pat : 1;
            UINT64 x3 : 4;
            UINT64 PageFrame : 40;
            UINT64 x4 : 7;
            UINT64 Key : 4;
            UINT64 x5 : 1;
        } Pte;

        struct
        {
            UINT64 x6 : 6;            // The 6 bits are always identical
            UINT64 Dirty : 1;
            UINT64 Pat : 1;
            UINT64 x7 : 4;
            UINT64 PageFrame : 40;
            UINT64 x8 : 12;
        } PtePae;
        
        struct
        {
            UINT64 x9 : 7;            // The bits are identical or reserved
            UINT64 LargePage : 1;     // Page size
            UINT64 x10 : 4;
            UINT64 PageFrame : 40;
            UINT64 x11 : 12;
        } Pde;

        struct
        {
            UINT64 x12 : 7;            // The bits are identical or reserved
            UINT64 LargePage : 1;      // Page size
            UINT64 x13 : 4;
            UINT64 PageFrame : 40;
            UINT64 x14 : 12;
        } PdePae;

        struct
        {
            UINT64 x15 : 6;            // The 6 bits are always identical
            UINT64 Dirty : 1;
            UINT64 LargePage : 1;
            UINT64 x16 : 4;
            UINT64 Pat : 1;
            UINT64 x17 : 8;
            UINT64 PageFrame : 31;   // Address of 2MB page frame.
            UINT64 x18 : 7;
            UINT64 Key : 4;
            UINT64 x19 : 1;
        } Pde2M;

        struct
        {
            UINT64 x20 : 6;            // The 6 bits are always identical
            UINT64 Dirty : 1;
            UINT64 LargePage : 1;
            UINT64 x21 : 4;
            UINT64 Pat : 1;
            UINT64 x22 : 8;
            UINT64 PageFrame : 31;    // Address of 2MB page frame.
            UINT64 x23 : 12;
        } Pde2MPae;

        UINT64 AsUint64;
    };
} PTE64;

typedef PTE64 PTE, *PPTE;
typedef PTE32 PTE_NOPAE, *PPTE_NOPAE;

#if defined(_WIN64)
typedef struct _MMPFN
{
     UINTN u1;
     PPTE PteAddress;
     PTE OriginalPte;
     UINTN u2;
     UINT64 u3;
     UINTN u4;
} MMPFN, *PMMPFN;
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
MmuMapIoPage(
    _In_ PHYSICAL_ADDRESS PhysicalPage,
    _In_ BOOLEAN Uncached
);

/**
* Unmaps (and frees an underlying resource) a previously
* mapped page. Page should have been mapped with MmuMapIoPage
*
* @param address [in] Previously returned virtual address.
*/
VOID
MmuUnmapIoPage(
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
MmuAddressToPte(
    _In_ PVOID Address
);

UINT16
MmuAddressToPti(
    _In_ UINT8 Level,
    _In_ PVOID Address
);

BOOLEAN
MmuAddressIsInPagingRange(
    _In_ PVOID Address
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
    _In_ PVOID Pml,
    _In_ UINT16 Index
);

UINT16
MmuGetPmlIndexFromOffset(
    _In_ UINT32 Offset
);

UINT8
MmuGetPagingLevels(
    VOID
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

VOID
MmuFinalize(
    VOID
);

#endif
