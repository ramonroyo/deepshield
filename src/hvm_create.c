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
            if (gHvm->VcpuArray[i].Stack)
                MemFree(gHvm->VcpuArray[i].Stack);

            if (gHvm->VcpuArray[i].VmcsRegionHva )
                MemFree(gHvm->VcpuArray[i].VmcsRegionHva );

            if (gHvm->VcpuArray[i].VmxOnRegionHva )
                MemFree(gHvm->VcpuArray[i].VmxOnRegionHva );
        }

        MemFree(gHvm->VcpuArray);
    }

    MemFree(gHvm);
    gHvm = 0;
}

#define XMM_REGISTER_ALIGNMENT_SIZE 16

NTSTATUS
HvmInitialize(
    _In_ UINT32 StackPages,
    _In_ PHVM_EXIT_HANDLER ExitHandlerCb,
    _In_ PHVM_SETUP_VMCS SetupVmcsCb
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

        gHvm->VcpuArray[i].Index = i;
        gHvm->VcpuArray[i].Hvm   = gHvm;

        gHvm->VcpuArray[i].VmxOnRegionHva = MemAllocAligned( PAGE_SIZE, PAGE_SIZE );
        if (!gHvm->VcpuArray[i].VmxOnRegionHva ) {
            goto failure;
        }

        gHvm->VcpuArray[i].VmcsRegionHva = MemAllocAligned( PAGE_SIZE, PAGE_SIZE );
        if (!gHvm->VcpuArray[i].VmcsRegionHva) {
            goto failure;
        }

        gHvm->VcpuArray[i].Stack = MemAllocAligned( StackPages * PAGE_SIZE, PAGE_SIZE );
        if (!gHvm->VcpuArray[i].Stack) {
            goto failure;
        }

        VcpuStackPointer = (PUINT_PTR)((UINT_PTR)gHvm->VcpuArray[i].Stack
                         + (StackPages * PAGE_SIZE)
                         - sizeof( UINT_PTR ));
        
        //
        //  Store the Vcpu at the end of the stack.
        //
        *VcpuStackPointer = (UINT_PTR)&gHvm->VcpuArray[i];

        //
        //  The stack must be 16-byte aligned for ABI compatibility with AMD64.
        //
        gHvm->VcpuArray[i].Rsp = ((UINT_PTR)gHvm->VcpuArray[i].Stack
                           + (StackPages * PAGE_SIZE) - 0x40);

        NT_ASSERT( (gHvm->VcpuArray[i].Rsp % 16) == 0 );

        gHvm->VcpuArray[i].ExitHandler = ExitHandlerCb;
        gHvm->VcpuArray[i].SetupVmcs = SetupVmcsCb;
        gHvm->VcpuArray[i].Launched  = FALSE;
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
