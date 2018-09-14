#include "wdk7.h"
#include "hvm.h"
#include "vmcs.h"
#include "vmx.h"
#include "x86.h"
#include "mmu.h"
#include "smp.h"

extern PHVM gHvm;

typedef struct DECLSPEC_ALIGN( 16 ) _IRET_FRAME
{
    UINTN rip;
    UINTN cs;
    UINTN rflags;
#ifdef _WIN64
    UINTN rsp;
    UINTN ss;
    UINTN align;
#endif
} IRET_FRAME, *PIRET_FRAME;

#define HVM_INTERNAL_SERVICE_STOP   0

extern NTSTATUS __stdcall
AsmHvmHyperCall(
    _In_ UINTN service,
    _In_ UINTN data
);

extern NTSTATUS __stdcall
AsmHvmLaunch(
    _In_ PHVM_VCPU Vcpu
);

extern VOID __stdcall
AsmHvmpStop(
    _In_ UINTN iret,
    _In_ UINTN Registers
);

extern VOID
AsmHvmpExitHandler(
    VOID
);

VOID
HvmpSaveHostState(
    _In_ PHOST_SAVED_STATE HostState
    )
{
    HostState->Cr0.AsUintN = __readcr0();
    HostState->Cr4.AsUintN = __readcr4();
    HostState->Cs.AsUint16 = AsmReadCs();
    HostState->Ss.AsUint16 = AsmReadSs();
    HostState->Ds.AsUint16 = AsmReadDs();
    HostState->Es.AsUint16 = AsmReadEs();
    
    HostState->Fs.AsUint16 = AsmReadFs();
#ifndef _WIN64
    HostState->FsBase      = BaseFromSelector(AsmReadFs());
#else
    HostState->FsBase      = __readmsr(IA32_FS_BASE);
#endif

    HostState->Gs.AsUint16    = AsmReadGs();
#ifndef _WIN64
    HostState->GsBase      = BaseFromSelector(AsmReadGs());
#else
    HostState->GsBase      = __readmsr(IA32_GS_BASE);
#endif

    HostState->Tr.AsUint16 = AsmReadTr();
    HostState->TrBase      = BaseFromSelector(AsmReadTr());
    AsmReadGdtr( &HostState->Gdt );
    HostState->Idt.Base    = GetIdtrBase();
    HostState->Idt.Limit   = GetIdtrLimit();
    HostState->SysenterCs  = (UINT32)__readmsr(IA32_SYSENTER_CS);
    HostState->SysenterEsp = (UINTN)__readmsr(IA32_SYSENTER_ESP);
    HostState->SysenterEip = (UINTN)__readmsr(IA32_SYSENTER_EIP);
    //HostState->perfGlobalCtrl;
    //HostState->pat;
    //HostState->efer;
}

NTSTATUS
VmxpEnable(
    PHYSICAL_ADDRESS VmxOn
    )
{
    CR4_REGISTER Cr4;

    //
    //  Activate VMX enable bit.
    //

    Cr4.AsUintN = __readcr4();
    Cr4.Bits.vmxe = 1;
    __writecr4( Cr4.AsUintN );

    if (AsmVmxOn( (PUINT64)&VmxOn) != 0 ) {

        Cr4.Bits.vmxe = 0;
        __writecr4( Cr4.AsUintN );

        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

VOID
VmxpDisable(
    _In_ PHYSICAL_ADDRESS Vmcs
    )
{
    CR4_REGISTER cr4;
    BOOLEAN Result;

    Result = (AsmVmxClear((PUINT64)&Vmcs) == 0);
    NT_ASSERT( Result );

    AsmVmxOff();

    cr4.AsUintN = __readcr4();
    cr4.Bits.vmxe = 0;
    __writecr4( cr4.AsUintN );
}

NTSTATUS __stdcall
HvmpStartVcpu(
    _In_ UINT32 VcpuId, 
    _In_ PVOID context
    )
{
    PHVM hvm;
    PHVM_VCPU Vcpu;
    NTSTATUS Status;

    hvm = (PHVM)context;
    Vcpu = &hvm->VcpuArray[VcpuId];

    AsmDisableInterrupts();

    Status = AsmHvmLaunch( Vcpu );

    AsmEnableInterrupts();

    return Status;
}

NTSTATUS __stdcall
HvmpEnterRoot(
    _In_ UINTN Code,
    _In_ UINTN Stack,
    _In_ UINTN Flags,
    _In_ PHVM_VCPU Vcpu
    )
{
    NTSTATUS Status;
    FLAGS_REGISTER RegFlags;

    NT_ASSERT( Vcpu );
    RegFlags.AsUintN = Flags;

    HvmpSaveHostState( &Vcpu->HostState );

    Status = VmxpEnable( Vcpu->VmxOnHpa );
    
    if (!NT_SUCCESS( Status )) {
        return Status;
    }

    //
    // Load current VMCS, after that we can access VMCS region.
    //

    if (AsmVmxClear((PUINT64)&Vcpu->VmcsHpa) != 0) {
        goto RoutineExit;
    }

    if (AsmVmxPtrLoad((PUINT64)&Vcpu->VmcsHpa) != 0) {
        goto RoutineExit;
    }

    Vcpu->SetupVmcs( Vcpu );

    VmxWritePlatform( HOST_RSP, Vcpu->Rsp );
    VmxWritePlatform( HOST_RIP, (UINTN)AsmHvmpExitHandler );
    VmcsConfigureCommonEntry( Code, Stack, RegFlags );
   
    //
    //  TODO: but it is not launched indeed!
    //
    AtomicWrite( &Vcpu->Launched, TRUE );

RoutineExit:

    if (!NT_SUCCESS( Status )) {
        VmxpDisable( Vcpu->VmcsHpa );
    }
    
    return Status;
}

NTSTATUS __stdcall
HvmpFailure(
    _In_ PHVM_VCPU Vcpu,
    _In_ BOOLEAN   valid
    )
{
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(Vcpu);

    if (valid) {
        Status = VMX_STATUS(VmxRead32( VM_INSTRUCTION_ERROR ));
    }
    else {
        Status = STATUS_UNSUCCESSFUL;
    }

    return Status;
}

NTSTATUS __stdcall
HvmpStartFailure(
    _In_ PHVM_VCPU Vcpu,
    _In_ BOOLEAN valid
    )
{
    NTSTATUS Status;

    AtomicWrite( &Vcpu->Launched, FALSE );
    Status = HvmpFailure( Vcpu, valid );
    
    VmxpDisable( Vcpu->VmcsHpa );

    return Status;
}

NTSTATUS __stdcall
HvmpStopVcpu(
    _In_ UINT32 VcpuId,
    _In_ PVOID context
    )
{
    UNREFERENCED_PARAMETER(VcpuId);
    UNREFERENCED_PARAMETER(context);

    return AsmHvmHyperCall( HVM_INTERNAL_SERVICE_STOP, 0 );
}

VOID
HvmpRestoreHostState(
    _In_ PHVM_VCPU Vcpu
    )
{
    CR4_REGISTER Cr4 = { 0 };

    //
    //  TODO: restore FULL state: CR0, CR4, fsBase, tr, sysenterCs ?
    //

    AsmWriteDs( Vcpu->HostState.Fs.AsUint16);
    AsmWriteEs( Vcpu->HostState.Es.AsUint16);
    AsmWriteFs( Vcpu->HostState.Fs.AsUint16);

#ifndef _WIN64
    AsmWriteSs( Vcpu->HostState.Ss.AsUint16);
    AsmWriteGs( Vcpu->HostState.Gs.AsUint16);
#endif

    AsmWriteGdtr( &Vcpu->HostState.Gdt );
    __lidt( &Vcpu->HostState.Idt );

    //
    //  Disable TSD monitoring.
    //
    Cr4.AsUintN = VmxReadPlatform( GUEST_CR4 );
    Cr4.Bits.Tsd = 0;
    __writecr4( Cr4.AsUintN );

    //
    //  To allow the stop routine being invoked in any process context the CR3
    //  should be set to the guest CR3.
    //
    __writecr3( VmxReadPlatform( GUEST_CR3 ) );
}

VOID
HvmpStop(
    _In_ PHVM_VCPU Vcpu,
    _In_ PGP_REGISTERS Registers
    )
{
#ifdef _WIN64
    IRET_FRAME iret;

    iret.rip = Registers->Rip;
    iret.cs = Vcpu->HostState.Cs.AsUint16;
    iret.rflags = Registers->Rflags.AsUintN;
    iret.rsp = Registers->Rsp;
    iret.ss = Vcpu->HostState.Ss.AsUint16;
#else

    PIRET_FRAME iretx86;

    iretx86 = (PIRET_FRAME) 
        ((PUINT_PTR) VmxReadPlatform( GUEST_RSP )
                     - (sizeof( IRET_FRAME ) / sizeof( UINTN )));

    iretx86->rip = Registers->Rip;
    iretx86->cs = Vcpu->HostState.Cs.AsUint16;
    iretx86->rflags = Registers->Rflags.AsUintN;
#endif
    
    //
    //  Restore segments, IDT and GDT from the saved state.
    //
    HvmpRestoreHostState( Vcpu );

    //
    //  This deactivates VMX operation in the processor.
    //
    VmxpDisable( Vcpu->VmcsHpa );

    //
    //  Restore registers and return from the interrupt.
    //
#ifdef _WIN64
    AsmHvmpStop((UINTN)&iret, (UINTN) Registers );
#else
    AsmHvmpStop( (UINTN)&iretx86, (UINTN) Registers );
#endif

}

NTSTATUS
HvmStart(
    VOID
    )
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    NT_ASSERT(gHvm);

    if (AtomicRead( &gHvm->launched ) == TRUE) {
        return Status;
    }

    Status = SmpRunPerProcessor( HvmpStartVcpu, gHvm );

    if (NT_SUCCESS( Status )) {
        AtomicWrite( &gHvm->launched, TRUE );
    }

    return Status;
}

NTSTATUS
HvmStop(
    VOID
    )
{
    NTSTATUS Status;

    Status = STATUS_UNSUCCESSFUL;

    if (gHvm == 0) {
        return Status;
    }

    if (AtomicRead( &gHvm->launched ) == FALSE) {
        return Status;
    }

    Status = SmpRunPerProcessor( HvmpStopVcpu, 0 );

    if (NT_SUCCESS( Status )) {
        AtomicWrite(&gHvm->launched, FALSE);
    }

    return Status;
}
