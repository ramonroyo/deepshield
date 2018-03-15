#ifndef __SHIELD_H__
#define __SHIELD_H__

#include <ntddk.h>

NTSTATUS
DsIsHvdsSupported(
    VOID
    );

NTSTATUS
DsInitializeHvds(
    VOID
    );

NTSTATUS
DsFinalizeHvds(
    VOID
    );

NTSTATUS
DsStartHvds(
    VOID
    );

NTSTATUS
DsStopHvds(
    VOID
    );

BOOLEAN
DsIsHvdsRunning(
    VOID
    );

#endif

