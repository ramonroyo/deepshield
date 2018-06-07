/**
*  @file    sync.h
*  @brief   Synchronization and SMP variables related primitives.
*/
#ifndef __SYNC_H__
#define __SYNC_H__

#include <ntifs.h>

/**
* Spinlock type. Natural word.
* Should be initialized to 0.
*/
#ifndef _WIN64
typedef volatile LONG SPINLOCK, *PSPINLOCK;
#else
typedef volatile LONGLONG SPINLOCK, *PSPINLOCK;
#endif

/**
* Tries to get the lock. Returns if successful or not but never blocks.
*/
BOOLEAN
SpinTryLock(
    _In_ PSPINLOCK spinlock
);

/**
* Waits until gets the lock. Can block.
*/
VOID
SpinLock(
    _In_ PSPINLOCK spinlock
);

/**
* Indicates if someone has obtained the lock or if it is free.
*/
BOOLEAN
SpinAcquired(
    _In_ PSPINLOCK spinlock
);

/**
* Frees the lock. The lock should be acquired previously or will block.
*/
VOID
SpinUnlock(
    _In_ PSPINLOCK spinlock
);


/**
* Condition variables. Natural word.
* Provides a value that can be used safely in SMP environments as provides core aquire and release memory fences
* both for reading and for writing.
*/
#ifndef _WIN64
typedef volatile LONG ATOMIC, *PATOMIC;
#else
typedef volatile LONGLONG ATOMIC, *PATOMIC;
#endif

ATOMIC
AtomicRead(
    _In_ PATOMIC atomic
);

VOID
AtomicWrite(
    _In_ PATOMIC atomic,
    _In_ ATOMIC  value
);


/**
* Semaphores
*/
typedef struct _SEMAPHORE
{
    SPINLOCK lock;
    ATOMIC   count;
} SEMAPHORE, *PSEMAPHORE;

VOID
SemaphoreSet(
    _In_ PSEMAPHORE semaphore,
    _In_ ATOMIC     threadCount
);

VOID
SemaphoreSignal(
    _In_ PSEMAPHORE semaphore
);

VOID
SemaphoreWait(
    _In_ PSEMAPHORE semaphore
);

/**
* Barrier type.
* Provides a "thread barrier" so at least a given number of threads has reached this point
* before all can continue.
*/
typedef struct _BARRIER
{
    ATOMIC    maxThreads;
    SPINLOCK  lock;
    ATOMIC    count;
    SEMAPHORE turnstile1;
    SEMAPHORE turnstile2;
} BARRIER, *PBARRIER;


/**
* Initializes/sets the barrier. Indicates the number of threads that will be using the barrier.
*/
VOID
BarrierSet(
    _In_ PBARRIER barrier,
    _In_ ATOMIC   threadCount
);

/**
* Waits in the barrier until all specified threads have reached this point. Then returns.
*/
VOID
BarrierEnter(
    _In_ PBARRIER barrier
);

/**
* Waits in the barrier until all specified threads have reached this point. Then returns.
*/
VOID
BarrierExit(
    _In_ PBARRIER barrier
);

#endif
