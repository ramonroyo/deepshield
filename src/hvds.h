#ifndef __SHIELD_H__
#define __SHIELD_H__

#include <ntifs.h>

NTSTATUS
DsIsHvdsSupported(
    VOID
    );

NTSTATUS
DsLoadHvds(
    VOID
    );

NTSTATUS
DsUnloadHvds(
    VOID
    );

BOOLEAN
DsIsHvdsRunning(
    VOID
    );

#endif

