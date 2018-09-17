#include "wdk7.h"
#include "hvm.h"
#include "vmx.h"
#include "mem.h"
#include "smp.h"
#include "vmcsinit.h"

NTSTATUS
HvmInitializePerCpu(
    _Inout_ PHVM Hvm,
    _In_ UINT32 StackPages,
    _In_ PHVM_EXIT_HANDLER ExitHandlerCb,
    _In_ PHVM_SETUP_VMCS SetupVmcsCb
    );

VOID
HvmDeletePerCpu(
    _Inout_ PHVM Hvm
    );

PHVM gHvm = NULL;

BOOLEAN
HvmInitialized(
    VOID
    )
{
    return (gHvm != NULL);
}

BOOLEAN
HvmLaunched(
    VOID
    )
{
    return HvmInitialized() && AtomicRead( &gHvm->launched );
}

VOID
HvmDestroy(
    VOID
    )
{
    if (!gHvm) {
        return;
    }

    HvmDeletePerCpu( gHvm );
    MemFree( gHvm );

    gHvm = NULL;
}

NTSTATUS
HvmInitializeVmxOn(
    _Inout_ PHVM_VCPU Vcpu
    )
{
    NTSTATUS Status = STATUS_NO_MEMORY;

    Vcpu->VmxOnHva = MemAllocAligned( PAGE_SIZE, PAGE_SIZE );
    if (Vcpu->VmxOnHva) {

        *(PUINT32)Vcpu->VmxOnHva = VMCS_REVISION;

        Vcpu->VmxOnHpa = MmGetPhysicalAddress( Vcpu->VmxOnHva );
        Status = STATUS_SUCCESS;
    }

    return Status;
}

VOID
HvmDeleteVmxOn(
    _Inout_ PHVM_VCPU Vcpu
    )
{
    MemFree( Vcpu->VmxOnHva );
}

NTSTATUS
HvmInitializeVmcs(
    _Inout_ PHVM_VCPU Vcpu
    )
{
    NTSTATUS Status = STATUS_NO_MEMORY;

    Vcpu->VmcsHva = VmcsAllocateRegion( VMCS_REGION_SIZE );
    if (Vcpu->VmcsHva) {

        Vcpu->VmcsHpa = MmGetPhysicalAddress( Vcpu->VmcsHva );
        Status = STATUS_SUCCESS;
    }

    return Status;
}

VOID
HvmDeleteVmcs(
    _Inout_ PHVM_VCPU Vcpu
    )
{
    MemFree( Vcpu->VmcsHva );
}

NTSTATUS
HvmInitializeStack(
    _Inout_ PHVM_VCPU Vcpu,
    _In_ UINT32 StackSize
    )
{
    NTSTATUS Status = STATUS_NO_MEMORY;
    PUCHAR StackFrame;

    StackFrame = (PUCHAR)MemAllocAligned( StackSize, PAGE_SIZE );
    if (StackFrame) {

        //
        //  Store the VCPU at the start of the stack frame.
        //
        *(PUINTN)(&StackFrame[StackSize - sizeof(UINTN)]) = (UINTN)&Vcpu;

        Vcpu->Stack = StackFrame;
        Status = STATUS_SUCCESS;
    }

    return Status;
}

VOID
HvmDeleteStack(
    _Inout_ PHVM_VCPU Vcpu
    )
{
    MemFree( Vcpu->Stack );
}

#define XMM_ALIGNMENT_SIZE 16

NTSTATUS
HvmInitializePerCpu(
    _Inout_ PHVM Hvm,
    _In_ UINT32 StackPages,
    _In_ PHVM_EXIT_HANDLER ExitHandlerCb,
    _In_ PHVM_SETUP_VMCS SetupVmcsCb
    )
{
    NTSTATUS Status = STATUS_NO_MEMORY;
    PHVM_VCPU CurrentVcpu;
    UINT32 StackSize;
    UINT32 RequestSize;
    UINT32 CpuCount;
    UINT32 CpuIdx;
       
    CpuCount = SmpActiveProcessorCount();
    RequestSize = CpuCount * sizeof( HVM_VCPU );

    Hvm->VcpuArray = MemAllocAligned( RequestSize, XMM_ALIGNMENT_SIZE );
    if (Hvm->VcpuArray == NULL) {
        return Status;
    }

    RtlZeroMemory( Hvm->VcpuArray, RequestSize );

    for (CpuIdx = 0; CpuIdx < CpuCount; CpuIdx++) {

        CurrentVcpu = &Hvm->VcpuArray[CpuIdx];

        CurrentVcpu->Hvm = Hvm;
        CurrentVcpu->Index = CpuIdx;
        CurrentVcpu->SetupVmcs = SetupVmcsCb;
        CurrentVcpu->ExitHandler = ExitHandlerCb;
        CurrentVcpu->Launched = FALSE;
        
        Status = HvmInitializeVmxOn( CurrentVcpu );
        if (!NT_SUCCESS( Status )) {
            goto RoutineExit;
        }

        Status = HvmInitializeVmcs( CurrentVcpu );
        if (!NT_SUCCESS( Status )) {
            goto RoutineExit;
        }

        StackSize = StackPages * PAGE_SIZE;

        Status = HvmInitializeStack( CurrentVcpu, StackSize );
        if (!NT_SUCCESS( Status )) {
            goto RoutineExit;
        }

        //
        //  The stack frame must be 16-byte aligned for ABI compatibility
        //  with AMD64.
        //
        CurrentVcpu->Rsp = (UINTN)CurrentVcpu->Stack + StackSize - 0x40;
        NT_ASSERT( (CurrentVcpu->Rsp % 16) == 0 );
    }

    Status = STATUS_SUCCESS;

RoutineExit:

    if (!NT_SUCCESS( Status )) {
        HvmDeletePerCpu( Hvm );
    }

    return Status;
}

VOID
HvmDeletePerCpu(
    _Inout_ PHVM Hvm
    )
{
    PHVM_VCPU CurrentVcpu;
    UINT32 CpuCount;
    UINT32 CpuIdx;

    if (Hvm->VcpuArray) {
        CpuCount = SmpActiveProcessorCount();

        for (CpuIdx = 0; CpuIdx < CpuCount; CpuIdx) {
            CurrentVcpu = &Hvm->VcpuArray[CpuIdx];

            if (CurrentVcpu->Stack) {
                HvmDeleteStack(CurrentVcpu);
            }

            if (CurrentVcpu->VmcsHva) {
                HvmDeleteVmcs(CurrentVcpu);
            }

            if (CurrentVcpu->VmxOnHva) {
                HvmDeleteVmxOn(CurrentVcpu);
            }
        }

        MemFree( Hvm->VcpuArray );
    }
}

NTSTATUS
HvmInitialize(
    _In_ UINT32 StackPages,
    _In_ PHVM_EXIT_HANDLER ExitHandlerCb,
    _In_ PHVM_SETUP_VMCS SetupVmcsCb
    )
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    PHVM Hvm = NULL;

    if (!NT_SUCCESS( VmxVerifySupport() )) {
        return STATUS_NOT_SUPPORTED;
    }

    if (HvmInitialized()) {
        if (AtomicRead( &gHvm->launched ) == TRUE) {
            return Status;
        }

        NT_ASSERT( FALSE );
    }

    Hvm = MemAlloc( sizeof( HVM ) );
    if (Hvm) {

        RtlZeroMemory( Hvm, sizeof( HVM ) );
        Status = HvmInitializePerCpu( Hvm,
                                      StackPages,
                                      ExitHandlerCb,
                                      SetupVmcsCb );
        if (NT_SUCCESS( Status )) {
            gHvm = Hvm;
        }
    }
    
    if (!NT_SUCCESS( Status )) {
        NT_ASSERT( FALSE == HvmInitialized() );

        if (Hvm) {
            MemFree( Hvm );
        }
    }

    return Status;
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
