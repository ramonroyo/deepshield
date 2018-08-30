#include "wdk7.h"
#include "hvds.h"
#include "exits.h"
#include "context.h"
#include "hvm.h"
#include "vmcs.h"
#include "vmx.h"
#include "mem.h"
#include "mmu.h"
#include "smp.h"

#define DS_VMM_STACK_PAGES 3

extern PGLOBAL_CONTEXT gGlobalContext;
extern PLOCAL_CONTEXT  gLocalContexts;

NTSTATUS
DspFinalizeHvds(
    VOID
    );

NTSTATUS __stdcall
LocalContextResetOnCore(
    _In_     UINT32 core,
    _In_opt_ PVOID  context
    )
{
    UNREFERENCED_PARAMETER(context);

    LocalContextReset(&gLocalContexts[core]);

    return STATUS_SUCCESS;
}

NTSTATUS __stdcall
LocalContextConfigureOnCore(
    _In_     UINT32 core,
    _In_opt_ PVOID  context
    )
{
    UNREFERENCED_PARAMETER(context);

    return LocalContextConfigure(&gLocalContexts[core]) ?
        STATUS_SUCCESS :
        STATUS_UNSUCCESSFUL;
}

VOID
DsConfigureHvds(
    _In_ PHVM_CORE core
    )
{
    PGLOBAL_CONTEXT globalContext;
    PLOCAL_CONTEXT  localContext;

    globalContext = (PGLOBAL_CONTEXT) HvmCoreGlobalContext(core);
    localContext  = (PLOCAL_CONTEXT)  HvmCoreLocalContext(core);

    //
    // Common configuration
    //
    VmcsConfigureCommonGuest();
    VmcsConfigureCommonHost( (UINT_PTR)globalContext->Cr3 );
    VmcsConfigureCommonControl();

    //
    // Unprivilege TSD
    //
    {
        CR4_REGISTER cr4 = { 0 };

        cr4.u.raw = VmxVmcsReadPlatform(GUEST_CR4);
        cr4.u.f.tsd = 1;

        VmxVmcsWritePlatform(GUEST_CR4, cr4.u.raw);
        VmxVmcsWritePlatform(CR4_GUEST_HOST_MASK, (1 << 2));
        VmxVmcsWritePlatform(CR4_READ_SHADOW, 0);
    }

    /*
    //
    // Follow CR3
    //
    {
        VMX_PROC_PRIMARY_CTLS procPrimaryControls;

        procPrimaryControls.u.raw = VmxVmcsRead32(VM_EXEC_CONTROLS_PROC_PRIMARY);
        procPrimaryControls.u.f.cr3LoadExiting = 1;
        VmxVmcsWrite32(VM_EXEC_CONTROLS_PROC_PRIMARY, procPrimaryControls.u.raw);
    }
    */


    //
    // Minimize msr exits
    //
    {
        PHYSICAL_ADDRESS msrBitmap;
        VMX_PROC_PRIMARY_CTLS procPrimaryControls;

        msrBitmap = MmuGetPhysicalAddress( 0, globalContext->msrBitmap );
        VmxVmcsWrite64( MSR_BITMAP_ADDRESS, msrBitmap.QuadPart );

        procPrimaryControls.u.raw = VmxVmcsRead32(VM_EXEC_CONTROLS_PROC_PRIMARY);
        procPrimaryControls.u.f.useMsrBitmaps = 1;
        VmxVmcsWrite32(VM_EXEC_CONTROLS_PROC_PRIMARY, procPrimaryControls.u.raw);
    }

    //
    // Activate #GP and #UD.
    //
    {
        VmxVmcsWrite32(EXCEPTION_BITMAP, (1 << TRAP_GP_FAULT) | (1 << TRAP_INVALID_OPCODE) );
    }
}

NTSTATUS
DsIsHvdsSupported(
    VOID
    )
{
    return VmxIsSupported();
}

NTSTATUS
DspInitializeHvds(
    VOID
    )
{
    NTSTATUS Status;
    UINT32 i;

    gGlobalContext = MemAlloc(sizeof(GLOBAL_CONTEXT));
    if (!gGlobalContext) {
        goto failure;
    }

    gLocalContexts = MemAllocArray(SmpNumberOfCores(), sizeof(LOCAL_CONTEXT));
    if (!gLocalContexts) {
        goto failure;
    }

    Status = HvmInitialize( DS_VMM_STACK_PAGES, 
                            DsHvdsExitHandler, 
                            DsConfigureHvds );

    if (!NT_SUCCESS( Status )) {
        goto failure;
    }

    if (!GlobalContextConfigure( gGlobalContext )) {
        goto failure;
    }

    if(!NT_SUCCESS( SmpExecuteOnAllCores( LocalContextConfigureOnCore, 0 ))) {
        goto failure;
    }

    HvmGlobalContextSet( gGlobalContext );

    for (i = 0; i < SmpNumberOfCores(); i++) {
        HvmLocalContextSet(i, &gLocalContexts[i]);
    }

    return STATUS_SUCCESS;

failure:
    DspFinalizeHvds();
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
DspFinalizeHvds(
    VOID
    )
{
    if (HvmInitialized()) {
        HvmFinalize();
    }

    if (gLocalContexts) {
        SmpExecuteOnAllCores(LocalContextResetOnCore, 0);

        MemFree(gLocalContexts);
        gLocalContexts = 0;
    }

    if(gGlobalContext) {
        GlobalContextReset(gGlobalContext);

        MemFree(gGlobalContext);
        gGlobalContext = 0;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
DsLoadHvds(
    VOID
    )
{
    NTSTATUS Status = DspInitializeHvds();

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
            DspFinalizeHvds();
        }
    }

    return Status;
}

NTSTATUS
DsUnloadHvds(
    VOID
    )
{
    NTSTATUS Status = HvmStop();

    if (NT_SUCCESS( Status )) {
        DspFinalizeHvds();
    }

    return Status;
}

BOOLEAN
DsIsHvdsRunning(
    VOID
    )
{
    return HvmLaunched();
}