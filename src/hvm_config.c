#include "wdk7.h"
#include "hvm.h"
#include "smp.h"

extern PHVM gHvm;

VOID
HvmGlobalContextSet(
    _In_ PVOID context
)
{
    if(!gHvm)
        return;

    InterlockedExchangePointer(&gHvm->globalContext, context);
}

PVOID
HvmGlobalContextGet(
    VOID
)
{
    if(!gHvm)
        return 0;

    return gHvm->globalContext;
}

VOID
HvmLocalContextSet(
    _In_ UINT32 VcpuId,
    _In_ PVOID context
   )
{
    if (!gHvm) {
        return;
    }

    if (VcpuId >= SmpActiveProcessorCount()) {
        return;
    }

    InterlockedExchangePointer(&gHvm->VcpuArray[VcpuId].LocalContext, context);
}

PVOID
HvmLocalContextGet(
    _In_ UINT32 VcpuId
   )
{
    if (!gHvm) {
        return 0;
    }

    if (VcpuId >= SmpActiveProcessorCount()) {
        return 0;
    }

    return gHvm->VcpuArray[VcpuId].LocalContext;
}
