#ifndef __HV_H__
#define __HV_H__

#include <ntddk.h>

NTSTATUS
HvInit(
    VOID
);

VOID
HvDone(
    VOID
);

NTSTATUS
HvStart(
    VOID
);

NTSTATUS
HvStop(
    VOID
);

NTSTATUS
HvLaunched(
    VOID
);

#endif
