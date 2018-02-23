#include "wdk7.h"
#include "sync.h"

BOOLEAN 
SpinTryLock(
    _In_ PSPINLOCK spinlock
)
{
#ifndef _WIN64
    return InterlockedCompareExchange(spinlock, 1, 0) == 0;
#else
    return InterlockedCompareExchange64(spinlock, 1, 0) == 0;
#endif
}


VOID
SpinLock(
    _In_ PSPINLOCK spinlock
)
{
    while (!SpinTryLock(spinlock))
        YieldProcessor();
}


BOOLEAN
SpinAcquired(
    _In_ PSPINLOCK spinlock
)
{
#ifndef _WIN64
    return InterlockedExchangeAdd(spinlock, 0) == 1;
#else
    return InterlockedAdd64(spinlock, 0) == 1;
#endif
}


VOID
SpinUnlock(
    _In_ PSPINLOCK spinlock
)
{
#ifndef _WIN64
    InterlockedCompareExchange(spinlock, 0, 1);
#else
    InterlockedCompareExchange64(spinlock, 0, 1);
#endif
}

ATOMIC
AtomicRead(
    _In_ PATOMIC atomic
)
{
#ifndef _WIN64
    return InterlockedExchangeAdd(atomic, 0);
#else
    return InterlockedAdd64(atomic, 0);
#endif
}

VOID
AtomicWrite(
    _In_ PATOMIC atomic,
    _In_ ATOMIC  value
)
{
#ifndef _WIN64
    InterlockedExchange(atomic, value);
#else
    InterlockedExchange64(atomic, value);
#endif
}

VOID
SemaphoreSet(
    _In_ PSEMAPHORE semaphore,
    _In_ ATOMIC     threadCount
)
{
    SpinLock(&semaphore->lock);

    AtomicWrite(&semaphore->count, threadCount);

    SpinUnlock(&semaphore->lock);
}

VOID
SemaphoreSignal(
    _In_ PSEMAPHORE semaphore
)
{
    ATOMIC count;

    SpinLock(&semaphore->lock);
    
    count = AtomicRead(&semaphore->count);
    AtomicWrite(&semaphore->count, count + 1);
    
    SpinUnlock(&semaphore->lock);
}

VOID
SemaphoreWait(
    _In_ PSEMAPHORE semaphore
)
{
    ATOMIC count;
    
    while(1)
    { 
        SpinLock(&semaphore->lock);

        count = AtomicRead(&semaphore->count);

        if(count >= 1)
        {
            AtomicWrite(&semaphore->count, count - 1);
            break;
        }

        SpinUnlock(&semaphore->lock);
    }

    SpinUnlock(&semaphore->lock);
}

VOID
BarrierSet(
    _In_ PBARRIER barrier,
    _In_ ATOMIC   threadCount
)
{
    SpinLock(&barrier->lock);

    AtomicWrite(&barrier->count, 0);
    AtomicWrite(&barrier->maxThreads, threadCount);
    SemaphoreSet(&barrier->turnstile1, 0);
    SemaphoreSet(&barrier->turnstile2, 1);

    SpinUnlock(&barrier->lock);
}


VOID
BarrierEnter(
    _In_ PBARRIER barrier
)
{
    ATOMIC count;

    SpinLock(&barrier->lock);

    count = AtomicRead(&barrier->count);
    count = count + 1;
    AtomicWrite(&barrier->count, count);

    if(count == AtomicRead(&barrier->maxThreads))
    {
        SemaphoreWait(&barrier->turnstile2);
        SemaphoreSignal(&barrier->turnstile1);
    }

    SpinUnlock(&barrier->lock);

    SemaphoreWait(&barrier->turnstile1);
    SemaphoreSignal(&barrier->turnstile1);
}


VOID
BarrierExit(
    _In_ PBARRIER barrier
)
{
    ATOMIC count;

    SpinLock(&barrier->lock);

    count = AtomicRead(&barrier->count);
    count = count - 1;
    AtomicWrite(&barrier->count, count);

    if(count == 0)
    {
        SemaphoreWait(&barrier->turnstile1);
        SemaphoreSignal(&barrier->turnstile2);
    }

    SpinUnlock(&barrier->lock);

    SemaphoreWait(&barrier->turnstile2);
    SemaphoreSignal(&barrier->turnstile2);
}
