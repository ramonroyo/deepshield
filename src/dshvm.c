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

#define DS_HVM_STACK_PAGES 3

static PHVM gHvm;
extern PHVM_CONTEXT gHvmContext;
extern PVCPU_CONTEXT gVcpuContexts;

VOID
DsDeleteHvmContexts(
    VOID
    );

NTSTATUS
DsFinalizeHvm(
    VOID
    );

VOID
DsHvmSetupVmcs(
    _In_ PHVM_VCPU Vcpu
    )
{
    PHYSICAL_ADDRESS MsrBitmap;
    PHVM_CONTEXT HvmContext = (PHVM_CONTEXT) HvmGetHvmContextByVcpu( Vcpu );

    VmcsSetHostField( HvmContext->SystemCr3 );
    VmcsSetControlField();
    VmcsSetGuestField();

    VmcsSetGuestPrivilegedTsd();

    //
    //  TODO: use conditionally.
    //
    MsrBitmap = MmuGetPhysicalAddress( 0, HvmContext->MsrBitmap );
    VmcsSetGuestNoMsrExits( MsrBitmap ); 
}

NTSTATUS
NTAPI
InitVcpuContextPerCpu(
    _In_ UINT32 VcpuId,
    _In_opt_ PVOID Reserved
    )
{
    UNREFERENCED_PARAMETER( Reserved );

    return InitializeVcpuContext( &gVcpuContexts[VcpuId] ) ?
        STATUS_SUCCESS :
        STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
ResetVcpuContextPerCpu(
    _In_ UINT32 VcpuId,
    _In_opt_ PVOID Reserved
    )
{
    UNREFERENCED_PARAMETER( Reserved );

    ResetVcpuContext( &gVcpuContexts[VcpuId] );
    return STATUS_SUCCESS;
}

NTSTATUS
DsInitializeHvmContexts(
    _Inout_ PHVM Hvm
    )
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    UINT32 i;

    gHvmContext = MemAlloc( sizeof(HVM_CONTEXT) );
    if (!gHvmContext) {
        goto RoutineExit;
    }

    if (!InitializeHvmContext( gHvmContext )) {
        goto RoutineExit;
    }

    HvmSetHvmContext( Hvm, gHvmContext );

    gVcpuContexts = MemAllocArray( SmpActiveProcessorCount(),
                                   sizeof( VCPU_CONTEXT ));
    if (!gVcpuContexts) {
        goto RoutineExit;
    }

    if(!NT_SUCCESS( SmpRunPerProcessor( InitVcpuContextPerCpu, NULL ))) {
        goto RoutineExit;
    }

    for (i = 0; i < SmpActiveProcessorCount(); i++) {
        HvmSetVcpuContext( Hvm, i, &gVcpuContexts[i] );
    }

    Status = STATUS_SUCCESS;

RoutineExit:

    if (!NT_SUCCESS( Status )) {
        DsDeleteHvmContexts();
    }

    return Status;
}

VOID
DsDeleteHvmContexts(
    VOID
    )
{
    if (gVcpuContexts) {
        SmpRunPerProcessor( ResetVcpuContextPerCpu, NULL );

        MemFree( gVcpuContexts );
        gVcpuContexts = NULL;
    }

    if (gHvmContext) {
        ResetHvmContext( gHvmContext );

        MemFree( gHvmContext );
        gHvmContext = NULL;
    }
}

DsIsHvmInitialized(
    VOID
    )
{
    return (gHvm != NULL);
}

BOOLEAN
DsIsHvmLaunched(
    VOID
    )
{
    return DsIsHvmInitialized() && AtomicRead( &gHvm->Launched );
}

NTSTATUS
DsInitializeHvm(
    VOID
    )
{
    NTSTATUS Status;
    PHVM Hvm = NULL;

    if (DsIsHvmInitialized()) {
        NT_ASSERT( FALSE == DsIsHvmLaunched() );
        NT_ASSERT( FALSE );
    }

    Status = HvmInitialize( DS_HVM_STACK_PAGES,
                            DsHvmExitHandler, 
                            DsHvmSetupVmcs,
                            &Hvm );

    if (NT_SUCCESS( Status )) {
        Status = DsInitializeHvmContexts( Hvm );

        if (!NT_SUCCESS( Status )) {
            HvmFinalize( Hvm );

        } else {
            gHvm = Hvm;
        }
    }

    return Status;
}

NTSTATUS
DsFinalizeHvm(
    VOID
    )
{
    if (DsIsHvmInitialized()) {
        NT_ASSERT( FALSE == DsIsHvmLaunched() );
        
        HvmFinalize( gHvm );
        gHvm = NULL;
    }

    DsDeleteHvmContexts();
    return STATUS_SUCCESS;
}

NTSTATUS
DsLoadHvm(
    VOID
    )
{
    NTSTATUS Status = DsInitializeHvm();

    if (NT_SUCCESS( Status )) {
        Status = HvmStart( gHvm );

        if (!NT_SUCCESS( Status ) ) {
            HvmStop( gHvm );
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
    NTSTATUS Status = HvmStop( gHvm );

    if (NT_SUCCESS( Status )) {
        DsFinalizeHvm();
    }

    return Status;
}