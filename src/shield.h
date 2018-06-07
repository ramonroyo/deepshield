#ifndef __HV_H__
#define __HV_H__

#include <ntifs.h>

NTSTATUS
DsInitializeShield(
    VOID
    );

VOID
DsFinalizeShield(
    VOID
    );

NTSTATUS
DsStartShield(
    VOID
    );

NTSTATUS
DsStopShield(
    VOID
    );

BOOLEAN
DsIsShieldRunning(
    VOID
    );

#endif