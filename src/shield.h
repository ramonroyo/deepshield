#ifndef __SHIELD_H__
#define __SHIELD_H__

#include <ntddk.h>

NTSTATUS
DeepShieldIsSupported(
    VOID
);

NTSTATUS
DeepShieldInit(
    VOID
);

NTSTATUS
DeepShieldDone(
    VOID
);

NTSTATUS
DeepShieldStart(
    VOID
);

NTSTATUS
DeepShieldStop(
    VOID
);

#endif
