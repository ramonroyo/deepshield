/*++

Copyright (c) ByteHeed. All Rights Reserved.

Module Name:

    spinlock.c

Abstract:

    This file implements the spinlock synchronizer.

Environment:

--*/

#include "dsdef.h"

FORCEINLINE
VOID
DspAcquireSpinLock(
    _In_ PLONG Lock
    )
{
    if (InterlockedBitTestAndSet( Lock, 0 )) {
        
        do {
            YieldProcessor();
        } while ((*(volatile LONG *)Lock != 0)
                || (InterlockedBitTestAndSet( Lock, 0) ));
    }
}

FORCEINLINE
VOID
DspReleaseSpinLock (
    _Inout_ PLONG Lock
    )
{
    InterlockedAnd( Lock, 0 );
}

FORCEINLINE
BOOLEAN
DspTryToAcquireSpinLock (
    _Inout_ PLONG Lock
    )
{
    if (*(volatile LONG *)Lock == 0) {
        return !InterlockedBitTestAndSet( Lock, 0 );
    }

    YieldProcessor();
    return FALSE;
}

VOID
DsInitializeSpinLock(
    _Inout_ PDS_PIN_LOCK SpinLock
    )
{
    SpinLock->Lock = 0;
    SpinLock->OldIrql = PASSIVE_LEVEL;
    SpinLock->Owner = NULL;
}

VOID
DsAcquireSpinLock(
    _Inout_ PDS_PIN_LOCK SpinLock
    )
{
    KIRQL OldIrql;

    KeRaiseIrql( HIGH_LEVEL, &OldIrql );
    DspAcquireSpinLock( &SpinLock->Lock );

    SpinLock->OldIrql = OldIrql;
    SpinLock->Owner = KeGetCurrentThread();
}

VOID
DsReleaseSpinLock(
    _Inout_ PDS_PIN_LOCK SpinLock
    )
{
    KIRQL OldIrql = SpinLock->OldIrql;

    //
    //  On release clear all acquirer data.
    //
    SpinLock->Owner = NULL;
    SpinLock->OldIrql = PASSIVE_LEVEL;

    DspReleaseSpinLock( &SpinLock->Lock );
    KeLowerIrql( OldIrql );
}

BOOLEAN
DsTryToAcquireSpinLock (
    _Inout_ PDS_PIN_LOCK SpinLock
    )
{
    KIRQL OldIrql;

    KeRaiseIrql( HIGH_LEVEL, &OldIrql );

    if (FALSE == DspTryToAcquireSpinLock( &SpinLock->Lock )) {
        KeLowerIrql( OldIrql );
        return FALSE;
    }

    SpinLock->Owner = KeGetCurrentThread();
    SpinLock->OldIrql = OldIrql;

    return TRUE;
}

BOOLEAN
DsTestSpinLock (
    _In_ PDS_PIN_LOCK SpinLock
    )
{
    KeMemoryBarrierWithoutFence();

    if (*(volatile LONG *)&SpinLock->Lock == 0) {
        return TRUE;
    }

    YieldProcessor();
    return FALSE;
}