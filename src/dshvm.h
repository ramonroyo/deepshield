#ifndef __SHIELD_H__
#define __SHIELD_H__

#include <ntifs.h>

NTSTATUS
DsLoadHvm(
    VOID
    );

NTSTATUS
DsUnloadHvm(
    VOID
    );

BOOLEAN
DsIsHvmLaunched(
    VOID
    );

#endif

