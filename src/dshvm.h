#ifndef __SHIELD_H__
#define __SHIELD_H__

#include <ntifs.h>

NTSTATUS
DsIsHvmSupported(
    VOID
    );

NTSTATUS
DsLoadHvm(
    VOID
    );

NTSTATUS
DsUnloadHvm(
    VOID
    );

BOOLEAN
DsIsHvmRunning(
    VOID
    );

#endif

