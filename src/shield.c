#include "wdk7.h"
#include "shield.h"
#include "exits.h"
#include "context.h"
#include "hvm.h"
#include "vmcs.h"
#include "vmx.h"
#include "mem.h"
#include "mmu.h"
#include "smp.h"

#define DEEPSHIELD_STACK_PAGES 2

extern PGLOBAL_CONTEXT gGlobalContext;
extern PLOCAL_CONTEXT  gLocalContexts;

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
DeepShieldConfigure(
    _In_ PHVM_CORE core
    )
{
    PGLOBAL_CONTEXT globalContext;
    PLOCAL_CONTEXT  localContext;

    globalContext = (PGLOBAL_CONTEXT)HvmCoreGlobalContext(core);
    localContext  = (PLOCAL_CONTEXT) HvmCoreLocalContext(core);

    //
    // Common configuration
    //
    VmcsConfigureCommonGuest();
    VmcsConfigureCommonHost();
    VmcsConfigureCommonControl();

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

        procPrimaryControls.u.raw = VmxVmcsRead32(VM_EXEC_CONTROLS_PROC_PRIMARY);
        procPrimaryControls.u.f.useMsrBitmaps = 1;
        VmxVmcsWrite32(VM_EXEC_CONTROLS_PROC_PRIMARY, procPrimaryControls.u.raw);

        msrBitmap = MmuGetPhysicalAddress(0, globalContext->msrBitmap);
        VmxVmcsWrite64(MSR_BITMAP_ADDRESS, msrBitmap.QuadPart);
    }

    //
    // Activate #PF on permissions check
    //
    {
        VmxVmcsWrite32(EXCEPTION_BITMAP, (1 << 14));
        VmxVmcsWrite32(PAGE_FAULT_ERRORCODE_MASK,  5);
        VmxVmcsWrite32(PAGE_FAULT_ERRORCODE_MATCH, 5);
    }
}

VOID
DeepShieldReset(
    VOID
    )
{
    if (HvmLaunched())
    {
        HvmStop();
    }

    if (HvmInitialized())
    {
        HvmDone();
    }

    if (gLocalContexts)
    {
        SmpExecuteOnAllCores(LocalContextResetOnCore, 0);

        MemFree(gLocalContexts);
        gLocalContexts = 0;
    }

    if(gGlobalContext)
    {
        GlobalContextReset(gGlobalContext);

        MemFree(gGlobalContext);
        gGlobalContext = 0;
    }
}

NTSTATUS
DeepShieldIsSupported(
    VOID
    )
{
    return VmxIsSupported();
}


NTSTATUS
DeepShieldInit(
    VOID
    )
{
    NTSTATUS status;
    UINT32 i;

    DeepShieldReset();

    gGlobalContext = MemAlloc(sizeof(GLOBAL_CONTEXT));
    if (!gGlobalContext)
        goto failure;

    gLocalContexts = MemAllocArray(SmpNumberOfCores(), sizeof(LOCAL_CONTEXT));
    if (!gLocalContexts)
        goto failure;

    status = HvmInit( DEEPSHIELD_STACK_PAGES,
                      DeepShieldExitHandler,
                      DeepShieldConfigure );

    if (!NT_SUCCESS( status ))
        goto failure;

    //
    // Configure
    //
    if (!GlobalContextConfigure(gGlobalContext))
        goto failure;

    if(!NT_SUCCESS(SmpExecuteOnAllCores(LocalContextConfigureOnCore, 0)))
        goto failure;

    //
    // Link
    //
    HvmGlobalContextSet(gGlobalContext);
    for (i = 0; i < SmpNumberOfCores(); i++)
    {
        HvmLocalContextSet(i, &gLocalContexts[i]);
    }

    return STATUS_SUCCESS;

failure:
    DeepShieldReset();
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
DeepShieldDone(
    VOID
)
{
    DeepShieldReset();
    return STATUS_SUCCESS;
}

NTSTATUS
DeepShieldStart(
    VOID
)
{
    return HvmStart();
}

NTSTATUS
DeepShieldStop(
    VOID
)
{
    return HvmStop();
}
