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

    if (gHvm->VcpuArray)
    {
        for (i = 0; i < SmpActiveProcessorCount(); i++)
        {
            if (gHvm->VcpuArray[i].stack)
                MemFree(gHvm->VcpuArray[i].stack);

            if (gHvm->VcpuArray[i].vmcs)
                MemFree(gHvm->VcpuArray[i].vmcs);

            if (gHvm->VcpuArray[i].vmxOn)
                MemFree(gHvm->VcpuArray[i].vmxOn);
        }

        MemFree(gHvm->VcpuArray);
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

    gHvm->VcpuArray = MemAllocAligned( SmpActiveProcessorCount() * sizeof( HVM_VCPU ),
                                   XMM_REGISTER_ALIGNMENT_SIZE );
    if (gHvm->VcpuArray == NULL)
    {
        MemFree(gHvm);
        gHvm = 0;
        return STATUS_UNSUCCESSFUL;
    }

    RtlZeroMemory( gHvm->VcpuArray, SmpActiveProcessorCount() * sizeof( HVM_VCPU ) );

    for (i = 0; i < SmpActiveProcessorCount(); i++)
    {
        PUINT_PTR VcpuStackPointer;

        gHvm->VcpuArray[i].index = i;
        gHvm->VcpuArray[i].hvm   = gHvm;

        gHvm->VcpuArray[i].vmxOn = MemAllocAligned( PAGE_SIZE, PAGE_SIZE );
        if (!gHvm->VcpuArray[i].vmxOn) {
            goto failure;
        }

        gHvm->VcpuArray[i].vmcs = MemAllocAligned( PAGE_SIZE, PAGE_SIZE );
        if (!gHvm->VcpuArray[i].vmcs) {
            goto failure;
        }

        gHvm->VcpuArray[i].stack = MemAllocAligned( stackPages * PAGE_SIZE, PAGE_SIZE );
        if (!gHvm->VcpuArray[i].stack) {
            goto failure;
        }

        VcpuStackPointer = (PUINT_PTR)((UINT_PTR)gHvm->VcpuArray[i].stack
                            + (stackPages * PAGE_SIZE) - sizeof(UINT_PTR));
        
        //
        //  Store the Vcpu at the end of the stack.
        //
        *VcpuStackPointer = (UINT_PTR)&gHvm->VcpuArray[i];

        //
        //  The stack must be 16-byte aligned for ABI compatibility with AMD64.
        //
        gHvm->VcpuArray[i].rsp = ((UINT_PTR)gHvm->VcpuArray[i].stack
                           + (stackPages * PAGE_SIZE) - 0x40);
        NT_ASSERT( (gHvm->VcpuArray[i].rsp % 16) == 0 );

        gHvm->VcpuArray[i].handler   = handler;
        gHvm->VcpuArray[i].configure = configure;
        
        gHvm->VcpuArray[i].launched  = FALSE;
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
