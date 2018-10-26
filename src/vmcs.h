#ifndef __VMCS_H__
#define __VMCS_H__

#include "x86.h"

#define BITARRAY_BYTE(n)    ((n) >> 3)
#define BITARRAY_MASK(n)    (1 << ((n) & 7))

#define BITARRAY_SET(a, n)  \
	(a[BITARRAY_BYTE(n)] |= BITARRAY_MASK(n))

#define BITARRAY_CLR(a, n)  \
	(a[BITARRAY_BYTE(n)] &= ~BITARRAY_MASK(n))

#define LOW32(v)  ((UINT32)(v))

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
VmcsSetGuestMsrExitPolicy(
    _In_ PUCHAR MsrBitmap
    );

#endif
