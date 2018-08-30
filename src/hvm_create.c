#include "wdk7.h"
#include "hvm.h"
#include "vmx.h"
#include "mem.h"
#include "smp.h"

PHVM gHvm = 0;

BOOLEAN
HvmInitialized(
    VOID
    )
{
    return gHvm != 0;
}

BOOLEAN
HvmLaunched(
    VOID
    )
{
    return HvmInitialized() && AtomicRead(&gHvm->launched);
}

VOID
HvmDestroy(
    VOID
    )
{
    UINT32 i;

    if (!gHvm) {
        return;
    }

    if (gHvm->cores)
    {
        for (i = 0; i < SmpNumberOfCores(); i++)
        {
            if (gHvm->cores[i].stack)
                MemFree(gHvm->cores[i].stack);

            if (gHvm->cores[i].vmcs)
                MemFree(gHvm->cores[i].vmcs);

            if (gHvm->cores[i].vmxOn)
                MemFree(gHvm->cores[i].vmxOn);
        }

        MemFree(gHvm->cores);
    }

    MemFree(gHvm);
    gHvm = 0;
}

#define XMM_REGISTER_ALIGNMENT_SIZE 16

NTSTATUS
HvmInitialize(
    _In_  UINT32 stackPages,
    _In_  HVM_EXIT_HANDLER handler,
    _In_  HVM_CONFIGURE configure
    )
{
    UINT32 i;

    if (!NT_SUCCESS( VmxIsSupported() )) {
        return STATUS_NOT_SUPPORTED;
    }

    if (gHvm != 0) {
        if (AtomicRead( &gHvm->launched ) == TRUE) {
            return STATUS_UNSUCCESSFUL;
        }

        HvmDestroy();
    }

    gHvm = MemAlloc(sizeof( HVM ) );
    if (!gHvm) {
        return STATUS_UNSUCCESSFUL;
    }

    RtlZeroMemory( gHvm, sizeof( HVM ) );

    gHvm->cores = MemAllocAligned( SmpNumberOfCores() * sizeof( HVM_CORE ),
                                   XMM_REGISTER_ALIGNMENT_SIZE );
    if (gHvm->cores == NULL)
    {
        MemFree(gHvm);
        gHvm = 0;
        return STATUS_UNSUCCESSFUL;
    }

    RtlZeroMemory( gHvm->cores, SmpNumberOfCores() * sizeof( HVM_CORE ) );

    for (i = 0; i < SmpNumberOfCores(); i++)
    {
        PUINT_PTR hvmCoreStackPointer;

        gHvm->cores[i].index = i;
        gHvm->cores[i].hvm   = gHvm;

        gHvm->cores[i].vmxOn = MemAllocAligned( PAGE_SIZE, PAGE_SIZE );
        if (!gHvm->cores[i].vmxOn) {
            goto failure;
        }

        gHvm->cores[i].vmcs = MemAllocAligned( PAGE_SIZE, PAGE_SIZE );
        if (!gHvm->cores[i].vmcs) {
            goto failure;
        }

        gHvm->cores[i].stack = MemAllocAligned( stackPages * PAGE_SIZE, PAGE_SIZE );
        if (!gHvm->cores[i].stack) {
            goto failure;
        }

        hvmCoreStackPointer = (PUINT_PTR)((UINT_PTR)gHvm->cores[i].stack 
                            + (stackPages * PAGE_SIZE) - sizeof(UINT_PTR));
        
        //
        //  Store the HvmCore at the end of the stack.
        //
        *hvmCoreStackPointer = (UINT_PTR)&gHvm->cores[i];

        //
        //  Keep it aligned to 64 bytes.
        //
        gHvm->cores[i].rsp = ((UINT_PTR)gHvm->cores[i].stack
                           + (stackPages * PAGE_SIZE) - 0x40);

        gHvm->cores[i].handler   = handler;
        gHvm->cores[i].configure = configure;
        
        gHvm->cores[i].launched  = FALSE;
    }

    return STATUS_SUCCESS;

failure:

    HvmDestroy();
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
HvmFinalize(
    VOID
    )
{
    if (gHvm != 0 && AtomicRead( &gHvm->launched ) == TRUE) {
        return STATUS_UNSUCCESSFUL;
    }

    HvmDestroy();
    return STATUS_SUCCESS;
}
