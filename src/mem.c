#include "wdk7.h"
#include "mem.h"
#include "sync.h"

/**
* Heap data structures
*/
#define HEAP_BLOCK_FREE 0
#define HEAP_BLOCK_USED 1

#pragma pack(push, 1)
typedef struct _HEAP
{
    SPINLOCK lock;     //Heap lock
    PVOID    memory;   //Pointer to raw memory
    UINT32   size;     //Size of raw memory
    UINT32   freeList; //Head of free blocks list (offset into [memory, memory + size) )
    UINT32   usedList; //Head of used blocks list (offset into [memory, memory + size) )
} HEAP, *PHEAP;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _HEAP_BLOCK
{
    UINT32 state;  //!< State of the block (eg: used or free)
    UINT32 size;   //!< Number of bytes of the block (available to the end user)
    UINT32 pprev;  //!< Offset of the previous physical block
    UINT32 pnext;  //!< Offset of the next physical block
    UINT32 vprev;  //!< Offset of the previous virtual block (in the same list as this: used or free)
    UINT32 vnext;  //!< Offset of the next virtual block (in the same list as this: used or free)
} HEAP_BLOCK, *PHEAP_BLOCK;
#pragma pack(pop)

#define BLOCK(X)    ((PHEAP_BLOCK)(((PUINT8)heap->memory) + (X) - sizeof(HEAP_BLOCK)))
#define STATE(X)    (BLOCK(X)->state)
#define SIZE(X)        (BLOCK(X)->size) 
#define PPREV(X)    (BLOCK(X)->pprev) 
#define PNEXT(X)    (BLOCK(X)->pnext)
#define VPREV(X)    (BLOCK(X)->vprev) 
#define VNEXT(X)    (BLOCK(X)->vnext)

#define START        sizeof(HEAP_BLOCK)

#define MINIMAL_FREE_BLOCK_SIZE    16

#define POINTER(X) ((PVOID)((PUINT8)heap->memory + (X)))          //!< Given an offset, converts it to a pointer in the heap
#define OFFSET(X)  ((UINT32)((PUINT8)(X) - (PUINT8)heap->memory)) //!< Given a pointer, converts it to a offset in the heap

/**
* Memory arena data structures
*/
typedef struct _MEMORY_ARENA
{
    PMDL   mdl;				//!< Physical pages description
    PVOID  virtualAddress;	//!< Virtual address where pages are mapped
    UINT32 numberOfPages;	//!< Allocated pages
} MEMORY_ARENA, *PMEMORY_ARENA;

VOID
HeappBlockUnlink(
    _In_    PHEAP   heap,
    _Inout_ UINT32* head,
    _In_    UINT32  address
)
{
    if (VNEXT(address))
    {
        VPREV(VNEXT(address)) = VPREV(address);
    }

    if (VPREV(address))
    {
        VNEXT(VPREV(address)) = VNEXT(address);
    }

    if (*head == address)
    {
        *head = VNEXT(address);
    }
}

VOID
HeappBlockLink(
    _In_    PHEAP   heap,
    _Inout_ UINT32* head,
    _In_    UINT32  address
)
{
    VNEXT(address) = *head;
    VPREV(address) = 0;

    if (*head)
    {
        VPREV(*head) = address;
    }

    *head = address;
}

VOID
HeappBlockZero(
    _In_ PHEAP  heap,
    _In_ UINT32 address
)
{
    memset(BLOCK(address), 0, sizeof(HEAP_BLOCK));
}

VOID
HeappSplit(
    _In_ PHEAP  heap,
    _In_ UINT32 address,
    _In_ UINT32 nbytes
)
{
    //
    //Can we split?
    //
    if (SIZE(address) >= nbytes + sizeof(HEAP_BLOCK) + MINIMAL_FREE_BLOCK_SIZE)
    {
        UINT32 newBlock;
        
        //
        //Create a new block
        //
        newBlock = address + nbytes + sizeof(HEAP_BLOCK);

        STATE(newBlock) = HEAP_BLOCK_FREE;
        SIZE(newBlock)  = SIZE(address) - nbytes - sizeof(HEAP_BLOCK);
        PPREV(newBlock) = address;
        PNEXT(newBlock) = PNEXT(address);
        HeappBlockLink(heap, &heap->freeList, newBlock);

        //
        //Insert in physical
        //
        if (PNEXT(address))
        {
            PPREV(PNEXT(address)) = newBlock;
        }
        PNEXT(address) = newBlock;

        //
        //Correct current size
        //
        SIZE(address) = nbytes;
    }

    //
    //Move from free to used
    //
    HeappBlockUnlink(heap, &heap->freeList, address);
    STATE(address) = HEAP_BLOCK_USED;
    HeappBlockLink(heap, &heap->usedList, address);
}

VOID
HeappAlign(
    _In_ PHEAP  heap,
    _In_ UINT32 address,
    _In_ UINT32 aligned,
    _In_ UINT32 numberOfBytes
)
{
    UINT32 gap;

    UNREFERENCED_PARAMETER(numberOfBytes);

    gap = aligned - address;
    
    //
    //Based on size of the gap, create a new free node or give space to the previous
    //
    if (gap >= (sizeof(HEAP_BLOCK) + MINIMAL_FREE_BLOCK_SIZE))
    {
        //
        //Create new node
        //
        STATE(aligned) = HEAP_BLOCK_FREE;
        SIZE(aligned)  = SIZE(address) - gap;
        PPREV(aligned) = address;
        PNEXT(aligned) = PNEXT(address);
        HeappBlockLink(heap, &heap->freeList, aligned);

        //
        //Relink prev and next
        //
        if (PNEXT(address))
        {
            PPREV(PNEXT(address)) = aligned;
        }
        PNEXT(address) = aligned;

        //
        //Correct prev size
        //
        SIZE(address) = gap - sizeof(HEAP_BLOCK);
    }
    else //Give to previous
    {
        HEAP_BLOCK tmp;

        //if (PPREV(_address)) //Guaranteed to exists
        //{
        
        //
        //Correct physical
        //
        if (PNEXT(address))
        {
            PPREV(PNEXT(address)) = aligned;
        }
        PNEXT(PPREV(address)) = aligned;

        //
        //Correct virtual
        //
        HeappBlockUnlink(heap, &heap->freeList, address);

        //
        //Copy
        //
        tmp = *BLOCK(address);
        *BLOCK(aligned) = tmp;

        //
        //Correct sizes
        //
        SIZE(PPREV(aligned)) += gap;
        SIZE(aligned) -= gap;

        HeappBlockLink(heap, &heap->freeList, aligned);
        //}
    }
}

VOID
HeappFree(
    _In_ PHEAP   heap,
    _In_ UINT32  current,
    _In_ BOOLEAN secure
)
{
    UINT32 prev;
    UINT32 next;
    PVOID  pointerToZero;
    UINT32 sizeToZero;

    //
    //More basic checks
    //
    if ((STATE(current) != HEAP_BLOCK_USED) || //region if free
        (PPREV(current) >= current)         || //previous element not previous
        (PNEXT(current) >= heap->size)      || //next is beyond the end
        (SIZE(current)  >= heap->size)      || //size region greater than whole
        (SIZE(current)  == 0)                  //no size at all
        )
        return;

    //
    //Coalesce
    //
    prev = PPREV(current);
    next = PNEXT(current);
    pointerToZero = POINTER(current);
    sizeToZero = SIZE(current);

    HeappBlockUnlink(heap, &heap->usedList, current);

    //
    // U U U => U F U
    //
    if (
        ((prev == 0) || (STATE(prev) == HEAP_BLOCK_USED))
        &&
        ((next == 0) || (STATE(next) == HEAP_BLOCK_USED))
        )
    {
        STATE(current) = HEAP_BLOCK_FREE;
        HeappBlockLink(heap, &heap->freeList, current);
    }

    //
    // U U F => U F F => U [F]
    //
    if (
        ((prev == 0) || (STATE(prev) == HEAP_BLOCK_USED))
        &&
        ((next != 0) && (STATE(next) == HEAP_BLOCK_FREE))
        )
    {
        HeappBlockUnlink(heap, &heap->freeList, next);
        SIZE(current) += sizeof(HEAP_BLOCK) + SIZE(next);
        PNEXT(current) = PNEXT(next);
        if (PNEXT(next))
        {
            PPREV(PNEXT(next)) = current;
        }
        if (secure)
            HeappBlockZero(heap, next);

        STATE(current) = HEAP_BLOCK_FREE;
        HeappBlockLink(heap, &heap->freeList, current);
    }

    //
    // F U U => F F U => [F] U
    //
    if (
        ((prev != 0) && (STATE(prev) == HEAP_BLOCK_FREE))
        &&
        ((next == 0) || (STATE(next) == HEAP_BLOCK_USED))
        )
    {
        SIZE(prev) += sizeof(HEAP_BLOCK) + SIZE(current);
        PNEXT(prev) = next;
        if (next)
            PPREV(next) = prev;

        if (secure)
            HeappBlockZero(heap, current);
    }

    //
    //F U F => F F F => [F]
    //
    if (
        ((prev != 0) && (STATE(prev) == HEAP_BLOCK_FREE))
        &&
        ((next != 0) && (STATE(next) == HEAP_BLOCK_FREE))
        )
    {
        HeappBlockUnlink(heap, &heap->freeList, next);

        SIZE(prev) += sizeof(HEAP_BLOCK) + SIZE(current) + sizeof(HEAP_BLOCK) + SIZE(next);
        PNEXT(prev) = PNEXT(next);
        if (PNEXT(next))
        {
            PPREV(PNEXT(next)) = prev;
        }

        if (secure)
        {
            HeappBlockZero(heap, current);
            HeappBlockZero(heap, next);
        }
    }

    if (secure)
    {
        memset(pointerToZero, 0, sizeToZero);
    }
}

BOOLEAN
HeappContains(
    _In_ PHEAP heap,
    _In_ PVOID address
)
{
    return (address >= (PVOID)((UINT8*)heap->memory + sizeof(HEAP_BLOCK))) && (address < (PVOID)((UINT8*)heap->memory + heap->size));
}



PHEAP
HeapCreate(
    _In_ PVOID  arena,
    _In_ UINT32 size
)
{
    PHEAP       heap;
    HEAP_BLOCK* start;

    if (size < 16 * PAGE_SIZE)
        return 0;

    heap = (PHEAP)arena;
    heap->memory   = (UINT8*)arena + PAGE_SIZE; //+ sizeof(HEAP); Page aligned so MemAllocAligned works correctly
    heap->size     = size - PAGE_SIZE;          //- sizeof(HEAP);
    heap->freeList = START;
    heap->usedList = 0;
    heap->lock     = 0;

    start = (HEAP_BLOCK*)heap->memory;
    start->size  = heap->size - sizeof(HEAP_BLOCK);
    start->state = HEAP_BLOCK_FREE;
    start->pnext = 0;
    start->pprev = 0;
    start->vnext = 0;
    start->vprev = 0;

    return heap;
}

VOID
HeapDelete(
    _In_ PHEAP heap
)
{
    SpinLock(&heap->lock);
    memset(heap, 0, heap->size + sizeof(HEAP));
}

PVOID
HeapAlloc(
    _In_ PHEAP  heap,
    _In_ UINT32 numberOfBytes
)
{
    UINT32 current;

    if (numberOfBytes == 0)
        return 0;

    SpinLock(&heap->lock);

    //
    // Search in the free list for a block with enough size
    //
    current = heap->freeList;
    while (current != 0)
    {
        //
        // Big enough?
        //
        if (SIZE(current) >= numberOfBytes)
        {
            //
            // Split to adjust size and leave a free block (if there is enough room)
            //
            HeappSplit(heap, current, numberOfBytes);

            SpinUnlock(&heap->lock);
            return POINTER(current);
        }

        current = VNEXT(current);
    }

    SpinUnlock(&heap->lock);
    return 0;
}


PVOID
HeapAllocAligned(
    _In_ PHEAP  heap,
    _In_ UINT32 numberOfBytes,
    _In_ UINT32 alignment
)
{
    UINT32 current;

    if (numberOfBytes == 0)
        return 0;

    if ((alignment == 0) || (alignment & (alignment - 1)))
        return 0;

    SpinLock(&heap->lock);

    //
    // Search in the free list for a block with enough size
    //
    current = heap->freeList;
    while (current != 0)
    {
        //
        // Big enough?
        //
        if ((SIZE(current) >= numberOfBytes))
        {
            UINT32 aligned;
            UINT32 gap;

            //
            // Calculate aligned offset from current
            //
            aligned = current / alignment;
            aligned++;
            aligned *= alignment;

            //
            // If current was already aligned...
            //
            if (aligned == current)
            {
                HeappSplit(heap, current, numberOfBytes);

                SpinUnlock(&heap->lock);
                return POINTER(current);
            }

            //
            // If there is no previous block to add the leftover space,
            // increase gap as necessary to create a minimal viable block
            //
            gap = aligned - current;
            if (PPREV(current) == 0)
            {
                while (gap < sizeof(HEAP_BLOCK) + MINIMAL_FREE_BLOCK_SIZE)
                {
                    gap += alignment;
                    aligned += alignment;
                }
            }

            //
            // If there is still room enough...
            //
            if (SIZE(current) >= (gap + numberOfBytes))
            {
                HeappAlign(heap, current, aligned, numberOfBytes);
                HeappSplit(heap, aligned, numberOfBytes);

                SpinUnlock(&heap->lock);
                return POINTER(aligned);
            }
        }

        current = VNEXT(current);
    }

    SpinUnlock(&heap->lock);
    return 0;
}

PVOID
HeapAllocArray(
    _In_ PHEAP  heap,
    _In_ UINT32 numberOfElements,
    _In_ UINT32 elementSize
)
{
    //PVOID array = HeapAllocAligned(heap, numberOfElements * elementSize, elementSize); //if elementSize is not power of two, would fail
    PVOID array = HeapAlloc(heap, numberOfElements * elementSize);
    if (array)
    {
        memset(array, 0, numberOfElements * elementSize);
    }
    return array;
}

VOID
HeapFree(
    _In_ PHEAP heap,
    _In_ PVOID memory
)
{
    //
    // Basic checks
    //
    if (!memory)
        return;

    SpinLock(&heap->lock);

    if (!HeappContains(heap, memory))
    {
        SpinUnlock(&heap->lock);
        return;
    }

    HeappFree(heap, OFFSET(memory), FALSE);

    SpinUnlock(&heap->lock);
}

VOID
HeapFreeSecure(
    _In_ PHEAP heap,
    _In_ PVOID memory
)
{
    UINT32  current;
    BOOLEAN found;
    UINT32  offset;

    //
    // Basic checks
    //
    if (!memory)
        return;

    SpinLock(&heap->lock);

    if (!HeappContains(heap, memory))
    {
        SpinUnlock(&heap->lock);
        return;
    }

    current = OFFSET(memory);

    //
    // Secure free for non-returned pointers
    // Search in the used list
    //
    found = FALSE;
    offset = heap->usedList;
    while (offset != 0)
    {
        if ((current >= offset) && (current < offset + SIZE(offset)))
        {
            if (current != offset)
                current = offset;

            found = TRUE;
            break;
        }

        offset = VNEXT(offset);
    }

    if (!found)
    {
        SpinUnlock(&heap->lock);
        return;
    }

    HeappFree(heap, current, TRUE);

    SpinUnlock(&heap->lock);
}

BOOLEAN
HeapContains(
    _In_ PHEAP heap,
    _In_ PVOID address
)
{
    BOOLEAN result = FALSE;

    SpinLock(&heap->lock);

    result = HeappContains(heap, address);

    SpinUnlock(&heap->lock);

    return result;
}




VOID
ArenaDone(
    _In_ PMEMORY_ARENA arena
);


NTSTATUS
ArenaInit(
    _In_ PMEMORY_ARENA arena,
    _In_ UINT32        numberOfBytes
)
{
    PHYSICAL_ADDRESS low;
    PHYSICAL_ADDRESS high;
    PHYSICAL_ADDRESS skip;

    low.QuadPart = 0;
    high.QuadPart = (UINT64)-1;
    skip.QuadPart = 0;

    //
    // Zero
    //
    memset(arena, 0, sizeof(MEMORY_ARENA));

    //
    // Initialize entry
    //
    arena->numberOfPages = BYTES_TO_PAGES(numberOfBytes);

    //
    // Allocate physical pages
    //
    arena->mdl = MmAllocatePagesForMdl(low, high, skip, arena->numberOfPages * PAGE_SIZE);
    if (!arena->mdl)
        goto failure;

    if (MmGetMdlByteCount(arena->mdl) != arena->numberOfPages * PAGE_SIZE)
        goto failure;

    //
    // Map pages
    //
    arena->virtualAddress = MmMapLockedPagesSpecifyCache(arena->mdl, KernelMode, MmNonCached, 0, FALSE, HighPagePriority /*| MdlMappingNoExecute*/);
    if (!arena->virtualAddress)
        goto failure;

    return STATUS_SUCCESS;

failure:
    ArenaDone(arena);

    return STATUS_UNSUCCESSFUL;
}

VOID
ArenaDone(
    _In_ PMEMORY_ARENA arena
)
{
    if (arena->virtualAddress)
    {
        MmUnmapLockedPages(arena->virtualAddress, arena->mdl);
    }

    if (arena->mdl)
    {
        MmFreePagesFromMdl(arena->mdl);
        ExFreePool(arena->mdl);
    }

    memset(arena, 0, sizeof(MEMORY_ARENA));
}

PHYSICAL_ADDRESS
ArenaVirtualToPhysical(
    _In_ PMEMORY_ARENA arena,
    _In_ PVOID         virtualAddress
)
{
    PHYSICAL_ADDRESS result;
    UINT_PTR         start;
    UINT_PTR         end;

    result.QuadPart = 0;
    start = (UINT_PTR)arena->virtualAddress;
    end   = (UINT_PTR)arena->virtualAddress + arena->numberOfPages * PAGE_SIZE;

    //Check virtual range
    if (start <= ((UINT_PTR)virtualAddress) && ((UINT_PTR)virtualAddress) < end)
    {
        UINT_PTR index = ((UINT_PTR)virtualAddress - start) >> 12;
        result.QuadPart = (MmGetMdlPfnArray(arena->mdl)[index] << 12) + ((UINT_PTR)virtualAddress & 0xFFF);
    }

    return result;
}

PVOID
ArenaPhysicalToVirtual(
    _In_ PMEMORY_ARENA    arena,
    _In_ PHYSICAL_ADDRESS physicalAddress
)
{
    PVOID       result;
    PPFN_NUMBER pfns;
    UINT32      entries;
    UINT32      i;

    result  = 0;
    pfns    = MmGetMdlPfnArray(arena->mdl);
    entries = ADDRESS_AND_SIZE_TO_SPAN_PAGES(MmGetMdlVirtualAddress(arena->mdl), MmGetMdlByteCount(arena->mdl));

    for (i = 0; i < entries; i++)
    {
        if (((UINT64)(physicalAddress.QuadPart >> 12)) == pfns[i])
        {
            result = (PVOID)(((UINT_PTR)arena->virtualAddress + i * PAGE_SIZE) + ((UINT_PTR)physicalAddress.QuadPart & 0xFFF));
            break;
        }
    }

    return result;
}



/*
*
* Global Memory System (malloc and the like replacement)
*
*/
MEMORY_ARENA gMemoryArena = { 0 };
PHEAP        gHeap = 0;

NTSTATUS
MemInit(
    _In_ UINT32 numberOfBytes
)
{
    NTSTATUS status;
    
    //
    // Create arena
    //
    status = ArenaInit(&gMemoryArena, numberOfBytes);
    if (!NT_SUCCESS(status))
        return status;

    //
    // Create heap
    //
    gHeap = HeapCreate(gMemoryArena.virtualAddress, gMemoryArena.numberOfPages * PAGE_SIZE);
    if (!gHeap)
    {
        ArenaDone(&gMemoryArena);
        status = STATUS_UNSUCCESSFUL;
    }

    return status;
}

VOID
MemDone(
    VOID
)
{
    HeapDelete(gHeap);
    ArenaDone(&gMemoryArena);
}

PVOID
MemAlloc(
    _In_ UINT32 numberOfBytes
)
{
    return HeapAlloc(gHeap, numberOfBytes);
}

PVOID
MemAllocAligned(
    _In_ UINT32 numberOfBytes,
    _In_ UINT32 alignment
)
{
    return HeapAllocAligned(gHeap, numberOfBytes, alignment);
}

PVOID
MemAllocArray(
    _In_ UINT32 numberOfElements,
    _In_ UINT32 elementSize
)
{
    return HeapAllocArray(gHeap, numberOfElements, elementSize);
}

VOID
MemFree(
    _In_ PVOID memory
)
{
    HeapFree(gHeap, memory);
}

VOID
MemFreeSecure(
    _In_ PVOID memory
)
{
    HeapFreeSecure(gHeap, memory);
}

PHYSICAL_ADDRESS
MemVirtualToPhysical(
    _In_ PVOID virtualAddress
)
{
    return ArenaVirtualToPhysical(&gMemoryArena, virtualAddress);
}

PVOID
MemPhysicalToVirtual(
    _In_ PHYSICAL_ADDRESS physicalAddress
)
{
    return ArenaPhysicalToVirtual(&gMemoryArena, physicalAddress);
}
