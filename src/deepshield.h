#ifndef __DEEPSHIELD_H__
#define __DEEPSHIELD_H__

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
