/**
*  @file    mem.h
*  @brief   Memory and Heap management functions
*/
#ifndef __MEM_H__
#define __MEM_H__

#include <ntddk.h>

typedef struct _HEAP* PHEAP;

/**
*  Creates a new heap on an already reserved  memory space.
*
* @param arena [in] Memory where heap will be constructed.
* @param size  [in] Size of the arena.
* @return A heap object prepared to be used. NULL on error.
*/
PHEAP
HeapCreate(
    _In_ PVOID  arena,
    _In_ UINT32 size
);

/**
*  Deletes a heap. Zeroes every bit. Does not free underlying arena.
*
* @param heap [in] Heap to be deleted.
*/
VOID
HeapDelete(
    _In_ PHEAP heap
);

/**
*  Allocates memory from a heap.
*
* @param heap          [in] Heap where memory is allocated from.
* @param numberOfBytes [in] Size of the allocation.
* @return Memory al least the size specified, NULL on error.
*/
PVOID
HeapAlloc(
    _In_ PHEAP  heap,
    _In_ UINT32 numberOfBytes
);

/**
*  Allocates memory from a heap, but aligned as specified
*
* @param heap          [in] Heap where memory is allocated from.
* @param numberOfBytes [in] Size of the allocation.
* @param alignment     [in] Alignment of the memory. Must be power of two until PAGE_SIZE
* @return Memory al least the size specified, aligned, or NULL on error.
*/
PVOID
HeapAllocAligned(
    _In_ PHEAP  heap,
    _In_ UINT32 numberOfBytes,
    _In_ UINT32 alignment
);

/**
*  Allocates memory from a heap, useful for arrays.
*
* @param heap             [in] Heap where memory is allocated from.
* @param numberOfElements [in] Number of elements of the array.
* @param elementSize      [in] Size of each element of the array.
* @return Memory al least the size (numberOfElements * elementSize), or NULL on error.
*/
PVOID
HeapAllocArray(
    _In_ PHEAP  heap,
    _In_ UINT32 numberOfElements,
    _In_ UINT32 elementSize
);

/**
*  Returns memory to a heap.
*
* @param heap   [in] Heap where memory is to be retuirned.
* @param memory [in] Previously allocated memory.
*/
VOID
HeapFree(
    _In_ PHEAP heap,
    _In_ PVOID memory
);

/**
*  Returns memory to a heap. Takes a number of verifications before 
*  so there are minor opportunity for the heap to get corrupted.
*  Also zeroes returned memory.
*
* @param heap   [in] Heap where memory is to be retuirned.
* @param memory [in] Previously allocated memory.
*/
VOID
HeapFreeSecure(
    _In_ PHEAP heap,
    _In_ PVOID memory
);

/**
*  Test if a pointer is contained into a given heap.
*
* @param heap    [in] Heap to test if memory belongs to.
* @param address [in] Pointer.
* @return TRUE if address belongs to heap, FALSE otherwise.
*/
BOOLEAN
HeapContains(
    _In_ PHEAP heap,
    _In_ PVOID address
);






typedef struct _MEMORY_ARENA* PMEMORY_ARENA;

/**
*  Initializes a memory arena.
*
* @param arena [in] Arena structure to initialize.
* @param numberOfBytes [in] Size of the arena.
* @return STATUS_SUCCESS os success, error code otherwise.
*/
NTSTATUS
ArenaInit(
    _In_ PMEMORY_ARENA arena,
    _In_ UINT32        numberOfBytes
);

/**
*  Deinitializes and deletes a memory arena.
*/
VOID
ArenaDone(
    _In_ PMEMORY_ARENA arena
);

/**
*  Translates to physical address a virtual address inside a given arena.
*
* @param arena [in] Arena structure.
* @param virtualAddress [in] Address to obtains its physical counterpart.
* @return Physical address os success, 0 otherwise.
*/
PHYSICAL_ADDRESS
ArenaVirtualToPhysical(
    _In_ PMEMORY_ARENA arena,
    _In_ PVOID         virtualAddress
);

/**
*  Translates to virtual address a physical address known to be inside a given arena.
*
* @param arena [in] Arena structure.
* @param physicalAddress [in] Address to obtains its virtual counterpart.
* @return Virtual address os success, 0 otherwise.
*/
PVOID
ArenaPhysicalToVirtual(
    _In_ PMEMORY_ARENA    arena,
    _In_ PHYSICAL_ADDRESS physicalAddress
);







/**
*  Initializes global memory system.
*  Does so by obtaining an arena from underlying OS and creating
*  a global heap on top of it
*
* @param numberOfBytes [in] Size of the global heap.
* @return STATUS_SUCCESS os success, error code otherwise.
*/
NTSTATUS
MemInit(
    _In_ UINT32 numberOfBytes
);

/**
*  Deinitializes global memory system.
*  Deletes global heap and frees underlying arena pages.
*/
VOID
MemDone(
    VOID
);

/**
*  Allocates memory from the global heap.
*  @see HeapAlloc
*
* @param numberOfBytes [in] Size of the allocation.
* @return Memory al least the size specified, NULL on error.
*/
PVOID
MemAlloc(
    _In_ UINT32 numberOfBytes
);

/**
*  Allocates memory from the global heap, but aligned as specified
*  @see HeapAllocAligned
*
* @param numberOfBytes [in] Size of the allocation.
* @param alignment     [in] Alignment of the memory. Must be power of two until PAGE_SIZE
* @return Memory al least the size specified, aligned, or NULL on error.
*/
PVOID
MemAllocAligned(
    _In_ UINT32 numberOfBytes,
    _In_ UINT32 alignment
);

/**
*  Allocates memory from the global heap, useful for arrays.
*  @see HeapAllocArray
*
* @param numberOfElements [in] Number of elements of the array.
* @param elementSize      [in] Size of each element of the array.
* @return Memory al least the size (numberOfElements * elementSize), or NULL on error.
*/
PVOID
MemAllocArray(
    _In_ UINT32 numberOfElements,
    _In_ UINT32 elementSize
);

/**
*  Returns memory to the global heap.
*  @see HeapFree
*
* @param memory [in] Previously allocated memory.
*/
VOID
MemFree(
    _In_ PVOID memory
);

/**
*  Returns memory to the global heap. Takes a number of verifications before
*  so there are minor opportunity for the heap to get corrupted.
*  Also zeroes returned memory.
*  @see HeapFreeSecure
*
* @param memory [in] Previously allocated memory.
*/
VOID
MemFreeSecure(
    _In_ PVOID memory
);


/**
*  Translates to physical address a virtual address inside global arena.
*
* @param virtualAddress [in] Address to obtains its physical counterpart.
* @return Physical address os success, 0 otherwise.
*/
PHYSICAL_ADDRESS
MemVirtualToPhysical(
    _In_ PVOID virtualAddress
);

/**
*  Translates to virtual address a physical address known to be inside the global arena.
*
* @param physicalAddress [in] Address to obtains its virtual counterpart.
* @return Virtual address os success, 0 otherwise.
*/
PVOID
MemPhysicalToVirtual(
    _In_ PHYSICAL_ADDRESS physicalAddress
);

#endif
