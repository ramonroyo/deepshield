#ifndef __CONTEXT_H__
#define __CONTEXT_H__

#include "hvm.h"

typedef struct _VCPU_CONTEXT
{
    PVOID TscHits;
    UINT64 TscOffset;
} VCPU_CONTEXT, *PVCPU_CONTEXT;

typedef struct _HVM_CONTEXT
{
    UINTN SystemCr3;
    PUCHAR MsrBitmap;
} HVM_CONTEXT, *PHVM_CONTEXT;

BOOLEAN
GlobalContextConfigure(
    _In_ PHVM_CONTEXT context
);

VOID
GlobalContextReset(
    _In_ PHVM_CONTEXT context
);

BOOLEAN
LocalContextConfigure(
    _In_ PVCPU_CONTEXT  local
);

VOID
LocalContextReset(
    _In_ PVCPU_CONTEXT  local
);


PHVM_CONTEXT
GlobalCtx(
    VOID
);

PVCPU_CONTEXT
LocalCtx(
    VOID
);


PVCPU_CONTEXT
LocalCtxForVcpu(
    _In_ UINT32 VcpuId
);


#endif
