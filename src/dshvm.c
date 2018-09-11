#include "wdk7.h"
#include "dshvm.h"
#include "exits.h"
#include "context.h"
#include "hvm.h"
#include "vmcs.h"
#include "vmcsinit.h"
#include "vmx.h"
#include "mem.h"
#include "mmu.h"
#include "smp.h"

#define DS_VMM_STACK_PAGES 3

extern PGLOBAL_CONTEXT gGlobalContext;
extern PLOCAL_CONTEXT  gLocalContexts;

NTSTATUS
DsFinalizeHvm(
    VOID
    );

NTSTATUS __stdcall
ResetLocalContextByVcpuId(
    _In_ UINT32 VcpuId,
    _In_opt_ PVOID Reserved
    )
{
    UNREFERENCED_PARAMETER( Reserved );

    LocalContextReset( &gLocalContexts[VcpuId] );

    return STATUS_SUCCESS;
}

NTSTATUS __stdcall
ConfigureLocalContextByVcpuId(
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
DsHvmSetupVmcs(
    _In_ PHVM_VCPU Vcpu
    )
{
    CR4_REGISTER cr4 = { 0 };
    PHYSICAL_ADDRESS msrBitmap;
    VMX_PROC_PRIMARY_CTLS procPrimaryControls;
    PGLOBAL_CONTEXT globalContext;
    PLOCAL_CONTEXT localContext;

    globalContext = (PGLOBAL_CONTEXT) HvmGetVcpuGlobalContext( Vcpu );
    localContext  = (PLOCAL_CONTEXT) HvmGetVcpuLocalContext( Vcpu );

    VmcsInitializeContext();

    //
    // Common configuration
    //
    VmcsConfigureCommonGuest();
    VmcsConfigureCommonHost( (UINT_PTR)globalContext->Cr3 );
    VmcsConfigureCommonControl();

    //
    //  Unprivilege TSD.
    //
    cr4.u.raw = VmxVmcsReadPlatform(GUEST_CR4);
    cr4.u.f.tsd = 1;

    VmxVmcsWritePlatform(GUEST_CR4, cr4.u.raw);
    VmxVmcsWritePlatform(CR4_GUEST_HOST_MASK, (1 << 2));
    VmxVmcsWritePlatform(CR4_READ_SHADOW, 0);

    //
    //  Minimize MSR exits.
    //

    msrBitmap = MmuGetPhysicalAddress( 0, globalContext->msrBitmap );
    VmxVmcsWrite64( MSR_BITMAP_ADDRESS, msrBitmap.QuadPart );

    procPrimaryControls.AsUint32 = VmxVmcsRead32( VM_EXEC_CONTROLS_PROC_PRIMARY );
    procPrimaryControls.Bits.useMsrBitmaps = 1;
    VmxVmcsWrite32 ( VM_EXEC_CONTROLS_PROC_PRIMARY, procPrimaryControls.AsUint32 );

    //
    //  Activate #GP and #UD.
    //
    VmxVmcsWrite32( EXCEPTION_BITMAP, (1 << VECTOR_INVALID_OPCODE_EXCEPTION)
                                    | (1 << VECTOR_GENERAL_PROTECTION_EXCEPTION) );
}

NTSTATUS
DsIsHvmSupported(
    VOID
    )
{
    return VmxIsSupported();
}

NTSTATUS
DsInitializeHvm(
    VOID
    )
{
    NTSTATUS Status;
    UINT32 i;

    gGlobalContext = MemAlloc(sizeof(GLOBAL_CONTEXT));
    if (!gGlobalContext) {
        goto failure;
    }

    gLocalContexts = MemAllocArray(SmpActiveProcessorCount(), sizeof(LOCAL_CONTEXT));
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

    if(!NT_SUCCESS( SmpExecuteOnAllProcessors( ConfigureLocalContextByVcpuId, NULL ))) {
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
        SmpExecuteOnAllProcessors(ResetLocalContextByVcpuId, 0);

        MemFree(gLocalContexts);
        gLocalContexts = 0;
    }

    if (gGlobalContext) {
        GlobalContextReset(gGlobalContext);

        MemFree(gGlobalContext);
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