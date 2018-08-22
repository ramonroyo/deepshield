#ifndef __VMCS_H__
#define __VMCS_H__

#include "x86.h"

BOOLEAN
VmcsClear(
    _In_ PVOID vmcs
);

BOOLEAN
VmcsLoad(
    _In_ PVOID vmcs
);

VOID
VmcsConfigureCommonGuest(
    VOID
);

VOID
VmcsConfigureCommonHost(
    _In_ UINT_PTR SystemCr3
);

VOID
VmcsConfigureCommonControl(
    VOID
);

VOID
VmcsConfigureCommonEntry(
    _In_ UINT_PTR       rip,
    _In_ UINT_PTR       rsp,
    _In_ FLAGS_REGISTER rflags
);

#endif
