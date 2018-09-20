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
VmcsConfigureCommonEntry(
    _In_ UINTN       rip,
    _In_ UINTN       rsp,
    _In_ FLAGS_REGISTER rflags
);

VOID
VmcsSetGuestPrivilegedTsd(
    VOID
    );

VOID
VmcsSetGuestNoMsrExits(
    _In_ PHYSICAL_ADDRESS MsrBitmap
    );

#endif
