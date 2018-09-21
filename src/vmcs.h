#ifndef __VMCS_H__
#define __VMCS_H__

#include "x86.h"

VOID
VmcsSetGuestField(
    VOID
    );

VOID
VmcsSetHostField(
    _In_ UINTN SystemCr3
    );

VOID
VmcsSetControlField(
    VOID
    );

VOID
VmcsSetCommonField(
    _In_ UINTN HostRsp,
    _In_ UINTN Rip,
    _In_ UINTN Rsp,
    _In_ UINTN Flags
    );

VOID
VmSetPrivilegedTimeStamp(
    VOID
    );

VOID
VmcsSetGuestNoMsrExits(
    _In_ PUCHAR MsrBitmap
    );

#endif
