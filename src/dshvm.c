#include "wdk7.h"
#include "dshvm.h"
#include "exits.h"
#include "context.h"
#include "hvm.h"
#include "vmcs.h"
#include "vmx.h"
#include "mem.h"
#include "mmu.h"
#include "smp.h"
#include "vmcsinit.h"

#define DS_VMM_STACK_PAGES 3

extern PHVM_CONTEXT gGlobalContext;
extern PVCPU_CONTEXT  gLocalContexts;

NTSTATUS
DsFinalizeHvm(
    VOID
    );

NTSTATUS __stdcall
ResetLocalContextPerCpu(
    _In_ UINT32 VcpuId,
    _In_opt_ PVOID Reserved
    )
{
    UNREFERENCED_PARAMETER( Reserved );

    LocalContextReset( &gLocalContexts[VcpuId] );

    return STATUS_SUCCESS;
}

NTSTATUS __stdcall
ConfigureLocalContextPerCpu(
    _In_ UINT32 Vcpu,
    _In_opt_ PVOID Reserved
    )
{
    UNREFERENCED_PARAMETER( Reserved );

    return LocalContextConfigure( &gLocalContexts[Vcpu] ) ?
        STATUS_SUCCESS :
        STATUS_UNSUCCESSFUL;
}

VOID
VmcsSetGuestPrivilegedTsd(
    VOID
    )
{
    CR4_REGISTER Cr4 = { 0 };

    //
    //  Deprivilege Tsd and make it host owned.
    //

    Cr4.AsUintN = VmReadN( GUEST_CR4 );
    Cr4.Bits.Tsd = 1;

    VmWriteN( GUEST_CR4, Cr4.AsUintN );
    VmWriteN( CR4_GUEST_HOST_MASK, (1 << 2) );
    VmWriteN( CR4_READ_SHADOW, 0 );

    //
    //  Activate #GP and #UD to reinject the Tsd trigered exceptions.
    //

    VmWrite32( EXCEPTION_BITMAP, (1 << VECTOR_INVALID_OPCODE_EXCEPTION)
                                | (1 << VECTOR_GENERAL_PROTECTION_EXCEPTION) );
}

VOID
VmcsSetGuestNoMsrExits(
    _In_ PHYSICAL_ADDRESS MsrBitmap
    )
{
    VMX_PROC_PRIMARY_CTLS ProcPrimaryControls;

    VmWrite64( MSR_BITMAP_ADDRESS, MsrBitmap.QuadPart );

    ProcPrimaryControls.AsUint32 = VmRead32( VM_EXEC_CONTROLS_PROC_PRIMARY );
    ProcPrimaryControls.Bits.UseMsrBitmap = 1;
    VmWrite32 ( VM_EXEC_CONTROLS_PROC_PRIMARY, ProcPrimaryControls.AsUint32 );
}

VOID
DsHvmSetupVmcs(
    _In_ PHVM_VCPU Vcpu
    )
{
    PHYSICAL_ADDRESS MsrBitmap;
    PHVM_CONTEXT HvmContext = (PHVM_CONTEXT)HvmGetHvmContext( Vcpu );
       
    VmcsSetHostField( HvmContext->SystemCr3 );
    VmcsSetGuestField();
    VmcsSetControlField();

    VmcsSetGuestPrivilegedTsd();

    // TODO: make conditionally.
    MsrBitmap = MmuGetPhysicalAddress( 0, HvmContext->MsrBitmap );
    VmcsSetGuestNoMsrExits( MsrBitmap ); 
}

NTSTATUS
DsIsHvmSupported(
    VOID
    )
{
    return VmxVerifySupport();
}

NTSTATUS
DsInitializeHvm(
    VOID
    )
{
    NTSTATUS Status;
    UINT32 i;

    gGlobalContext = MemAlloc(sizeof(HVM_CONTEXT));
    if (!gGlobalContext) {
        goto failure;
    }

    gLocalContexts = MemAllocArray(SmpActiveProcessorCount(), sizeof(VCPU_CONTEXT));
    if (!gLocalContexts) {
        goto failure;
    }

    Status = HvmInitialize( DS_VMM_STACK_PAGES, DsHvmExitHandler, DsHvmSetupVmcs );

    if (!NT_SUCCESS( Status )) {
        goto failure;
    }

    if (!GlobalContextConfigure( gGlobalContext )) {
        goto failure;
    }

    if(!NT_SUCCESS( SmpRunPerProcessor(ConfigureLocalContextPerCpu, NULL ))) {
        goto failure;
    }

    HvmGlobalContextSet( gGlobalContext );

    for (i = 0; i < SmpActiveProcessorCount(); i++) {
        HvmLocalContextSet(i, &gLocalContexts[i]);
    }

    return STATUS_SUCCESS;

failure:
    DsFinalizeHvm();
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
DsFinalizeHvm(
    VOID
    )
{
    if (HvmInitialized()) {
        HvmFinalize();
    }

    if (gLocalContexts) {
        SmpRunPerProcessor( ResetLocalContextPerCpu, 0 );

        MemFree(gLocalContexts);
        gLocalContexts = 0;
    }

    if (gGlobalContext) {
        GlobalContextReset( gGlobalContext );

        MemFree( gGlobalContext );
        gGlobalContext = 0;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
DsLoadHvm(
    VOID
    )
{
    NTSTATUS Status = DsInitializeHvm();

    if (NT_SUCCESS( Status )) {

        //
        //  Start the virtual machine.
        //
        Status = HvmStart();

        if (!NT_SUCCESS( Status ) ) {

            //
            //  Cleanup on partial start.
            //
            HvmStop();

            //
            //  Deallocate resources.
            //
            DsFinalizeHvm();
        }
    }

    return Status;
}

NTSTATUS
DsUnloadHvm(
    VOID
    )
{
    NTSTATUS Status = HvmStop();

    if (NT_SUCCESS( Status )) {
        DsFinalizeHvm();
    }

    return Status;
}

BOOLEAN
DsIsHvmRunning(
    VOID
    )
{
    return HvmLaunched();
}