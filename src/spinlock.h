/*++

Copyright (C) ByteHeed. All Rights Reserved

Module Name:

    spinlock.h

--*/

#ifndef _SPINLOCK_
#define _SPINLOCK_

#include <ntifs.h>
#include "wdk7.h"

typedef struct _DS_SPIN_LOCK {
    LONG Lock;
    KIRQL OldIrql;
    PKTHREAD Owner;
} DS_SPIN_LOCK, *PDS_PIN_LOCK;

VOID
DsInitializeSpinLock(
    _Inout_ PDS_PIN_LOCK SpinLock
    );

VOID
DsAcquireSpinLock(
    _Inout_ PDS_PIN_LOCK SpinLock
    );

VOID
DsReleaseSpinLock(
    _Inout_ PDS_PIN_LOCK SpinLock
    );

BOOLEAN
DsTryToAcquireSpinLock(
    _Inout_ PDS_PIN_LOCK SpinLock
    );

BOOLEAN
DsTestSpinLock(
    _In_ PDS_PIN_LOCK SpinLock
    );

#endif