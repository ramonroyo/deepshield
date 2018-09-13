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
    UINT_PTR rip;
    UINT_PTR cs;
    UINT_PTR rflags;
#ifdef _WIN64
    UINT_PTR rsp;
    UINT_PTR ss;
    UINT_PTR align;
#endif
} IRET_FRAME, *PIRET_FRAME;

#define HVM_INTERNAL_SERVICE_STOP   0

extern NTSTATUS __stdcall
HvmInternalCallAsm(
    _In_ UINT_PTR service,
    _In_ UINT_PTR data
);

extern NTSTATUS __stdcall
HvmpStartAsm(
    _In_ PHVM_VCPU Vcpu
);

extern VOID __stdcall
HvmpStopAsm(
    _In_ UINT_PTR iret,
    _In_ UINT_PTR Registers
);

extern VOID
HvmpExitHandlerAsm(
    VOID
);


VOID
HvmpSaveHostState(
    _In_ PHOST_SAVED_STATE state
)
{
    state->cr0.u.raw   = __readcr0();
    state->cr4.AsUintN = __readcr4();
    state->cs.u.raw    = AsmReadCs();
    state->ss.u.raw    = AsmReadSs();
    state->ds.u.raw    = AsmReadDs();
    state->es.u.raw    = AsmReadEs();
    state->fs.u.raw    = AsmReadFs();
#ifndef _WIN64
    state->fsBase      = DescriptorBase(AsmReadFs());
#else
    state->fsBase      = __readmsr(IA32_FS_BASE);
#endif    
    state->gs.u.raw    = AsmReadGs();
#ifndef _WIN64
    state->gsBase      = DescriptorBase(AsmReadGs());
#else
    state->gsBase      = __readmsr(IA32_GS_BASE);
#endif    
    state->tr.u.raw    = AsmReadTr();
    state->trBase      = DescriptorBase(AsmReadTr());
    state->gdt.base    = sgdt_base();
    state->gdt.limit   = sgdt_limit();
    state->idt.base    = sidt_base();
    state->idt.limit   = sidt_limit();
    state->sysenterCs  = (UINT32)  __readmsr(IA32_SYSENTER_CS);
    state->sysenterEsp = (UINT_PTR)__readmsr(IA32_SYSENTER_ESP);
    state->sysenterEip = (UINT_PTR)__readmsr(IA32_SYSENTER_EIP);
    //state->perfGlobalCtrl;
    //state->pat;
    //state->efer;
}

NTSTATUS
VmxpEnable(
    PVOID vmxOn
)
{
    CR4_REGISTER Cr4;
    PHYSICAL_ADDRESS VmxOnPhysical;

    //
    //  Activate VMX enable bit.
    //

    Cr4.AsUintN = __readcr4();
    Cr4.Bits.vmxe = 1;
    __writecr4(Cr4.AsUintN );

    VmxOnPhysical = MmuGetPhysicalAddress( 0, vmxOn );

    if (VmxOn( (UINT64*)&VmxOnPhysical) != 0 ) {

        Cr4.Bits.vmxe = 0;
        __writecr4( Cr4.AsUintN );

        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

VOID
VmxpDisable(
    _In_ PHYSICAL_ADDRESS VmcsHpa
    )
{
    CR4_REGISTER cr4;
    BOOLEAN ClearResult;

    ClearResult = VmcsClear( VmcsHpa );
    NT_ASSERT( ClearResult );

    VmxOff();

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
    NTSTATUS status;

    hvm = (PHVM)context;
    Vcpu = &hvm->VcpuArray[VcpuId];

    AsmDisableInterrupts();

    status = HvmpStartAsm( Vcpu );

    AsmEnableInterrupts();

    return status;
}

NTSTATUS __stdcall
HvmpEnterRoot(
    _In_ UINT_PTR Code,
    _In_ UINT_PTR Stack,
    _In_ UINT_PTR Flags,
    _In_ PHVM_VCPU Vcpu
    )
{
    NTSTATUS Status;
    FLAGS_REGISTER rflags;
    VMX_MSR_BASIC BasicVmx;

    NT_ASSERT( Vcpu );
    rflags.u.raw = Flags;

    BasicVmx.AsUint64 = VmxCapability( IA32_VMX_BASIC );

    *(PUINT32) Vcpu->VmxOnRegionHva = BasicVmx.Bits.revisionId;

    HvmpSaveHostState( &Vcpu->SavedState );

    Status = VmxpEnable( Vcpu->VmxOnRegionHva );
    
    if (!NT_SUCCESS( Status )) {
        return Status;
    }

    if (!VmcsClear( Vcpu->VmcsRegionHpa )) {
        goto RoutineExit;
    }

    if (!VmcsLoad( Vcpu->VmcsRegionHpa )) {
        goto RoutineExit;
    }

    Vcpu->SetupVmcs( Vcpu );

    VmxWritePlatform( HOST_RSP, Vcpu->Rsp );
    VmxWritePlatform( HOST_RIP, (UINT_PTR)HvmpExitHandlerAsm );
    VmcsConfigureCommonEntry( Code, Stack, rflags );
   
    //
    //  TODO: but it is not launched indeed!
    //
    AtomicWrite( &Vcpu->Launched, TRUE );

RoutineExit:

    if (!NT_SUCCESS( Status )) {
        VmxpDisable( Vcpu->VmcsRegionHpa );
    }
    
    return Status;
}

NTSTATUS __stdcall
HvmpFailure(
    _In_ PHVM_VCPU Vcpu,
    _In_ BOOLEAN   valid
    )
{
    NTSTATUS status;

    UNREFERENCED_PARAMETER(Vcpu);

    if (valid) {
        status = VMX_STATUS(VmxRead32( VM_INSTRUCTION_ERROR ));
    }
    else {
        status = STATUS_UNSUCCESSFUL;
    }

    return status;
}

NTSTATUS __stdcall
HvmpStartFailure(
    _In_ PHVM_VCPU Vcpu,
    _In_ BOOLEAN valid
    )
{
    NTSTATUS status;

    AtomicWrite( &Vcpu->Launched, FALSE );
    status = HvmpFailure( Vcpu, valid );
    
    VmxpDisable( Vcpu->VmcsRegionHpa );

    return status;
}

NTSTATUS __stdcall
HvmpStopVcpu(
    _In_ UINT32 VcpuId,
    _In_ PVOID context
    )
{
    UNREFERENCED_PARAMETER(VcpuId);
    UNREFERENCED_PARAMETER(context);

    return HvmInternalCallAsm( HVM_INTERNAL_SERVICE_STOP, 0 );
}

VOID
HvmpRestore(
    _In_ PHVM_VCPU Vcpu
    )
{
    CR4_REGISTER Cr4 = { 0 };

    //
    //  TODO: restore FULL state: CR0, CR4, fsBase, tr, sysenterCs ?
    //

    AsmWriteDs( Vcpu->SavedState.ds.u.raw );
    AsmWriteEs( Vcpu->SavedState.es.u.raw );
    AsmWriteFs( Vcpu->SavedState.fs.u.raw );
#ifndef _WIN64
    AsmWriteSs( Vcpu->SavedState.ss.u.raw );
    AsmWriteGs( Vcpu->SavedState.gs.u.raw );
#endif

    lgdt( Vcpu->SavedState.gdt.base, Vcpu->SavedState.gdt.limit );
    lidt( Vcpu->SavedState.idt.base, Vcpu->SavedState.idt.limit );

    //
    //  Disable TSD monitoring.
    //
    Cr4.AsUintN = VmxReadPlatform( GUEST_CR4 );
    Cr4.Bits.tsd = 0;
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

    iret.rip = Registers->rip;
    iret.cs = Vcpu->SavedState.cs.u.raw;
    iret.rflags = Registers->rflags.u.raw;
    iret.rsp = Registers->rsp;
    iret.ss = Vcpu->SavedState.ss.u.raw;
#else

    PIRET_FRAME iretx86;

    iretx86 = (PIRET_FRAME) 
        ((PUINT_PTR) VmxReadPlatform( GUEST_RSP )
                     - (sizeof( IRET_FRAME ) / sizeof( UINT_PTR )));

    iretx86->rip = Registers->rip;
    iretx86->cs = Vcpu->SavedState.cs.u.raw;
    iretx86->rflags = Registers->rflags.u.raw;
#endif
    
    //
    //  Restore segments, IDT and GDT from the saved state.
    //
    HvmpRestore( Vcpu );

    //
    //  This deactivates VMX operation in the processor.
    //
    VmxpDisable( Vcpu->VmcsRegionHpa );

    //
    //  Restore registers and return from the interrupt.
    //
#ifdef _WIN64
    HvmpStopAsm((UINT_PTR)&iret, (UINT_PTR) Registers );
#else
    HvmpStopAsm( (UINT_PTR)&iretx86, (UINT_PTR) Registers );
#endif

}

NTSTATUS
HvmStart(
    VOID
    )
{
    NTSTATUS status;

    status = STATUS_UNSUCCESSFUL;

    if (gHvm == 0) {
        return status;
    }

    if (AtomicRead( &gHvm->launched ) == TRUE) {
        return status;
    }

    status = SmpExecuteOnAllProcessors( HvmpStartVcpu, gHvm );

    if (NT_SUCCESS( status )) {
        AtomicWrite( &gHvm->launched, TRUE );
    }

    return status;
}

NTSTATUS
HvmStop(
    VOID
    )
{
    NTSTATUS status;

    status = STATUS_UNSUCCESSFUL;

    if (gHvm == 0) {
        return status;
    }

    if (AtomicRead( &gHvm->launched ) == FALSE) {
        return status;
    }

    status = SmpExecuteOnAllProcessors( HvmpStopVcpu, 0 );

    if (NT_SUCCESS( status )) {
        AtomicWrite(&gHvm->launched, FALSE);
    }

    return status;
}
