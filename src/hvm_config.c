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
    _In_ UINT32 core,
    _In_ PVOID  context
)
{
    if(!gHvm)
        return;

    if(core >= SmpNumberOfCores())
        return;

    InterlockedExchangePointer(&gHvm->cores[core].localContext, context);
}

PVOID
HvmLocalContextGet(
    _In_ UINT32 core
)
{
    if(!gHvm)
        return 0;

    if(core >= SmpNumberOfCores())
        return 0;

    return gHvm->cores[core].localContext;
}
