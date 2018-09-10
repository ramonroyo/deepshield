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

#ifdef _WIN64
typedef struct DECLSPEC_ALIGN(16) _HVM_STOP_STACK_LAYOUT {
    REGISTERS  regs;
    IRET_FRAME iret;
} HVM_STOP_STACK_LAYOUT, *PHVM_STOP_STACK_LAYOUT;
#else
typedef struct _HVM_STOP_STACK_LAYOUT {
    REGISTERS  regs;
    PIRET_FRAME iret;
} HVM_STOP_STACK_LAYOUT, *PHVM_STOP_STACK_LAYOUT;
#endif


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
    _In_ UINT_PTR regs
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
    state->cr4.u.raw   = __readcr4();
    state->cs.u.raw    = __readcs();
    state->ss.u.raw    = __readss();
    state->ds.u.raw    = __readds();
    state->es.u.raw    = __reades();
    state->fs.u.raw    = __readfs();
#ifndef _WIN64
    state->fsBase      = DescriptorBase(__readfs());
#else
    state->fsBase      = __readmsr(IA32_FS_BASE);
#endif    
    state->gs.u.raw    = __readgs();
#ifndef _WIN64
    state->gsBase      = DescriptorBase(__readgs());
#else
    state->gsBase      = __readmsr(IA32_GS_BASE);
#endif    
    state->tr.u.raw    = __str();
    state->trBase      = DescriptorBase(__str());
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
    VMX_MSR_BASIC        basicVmx;
    CR4_REGISTER     cr4;
    PHYSICAL_ADDRESS vmxOnPhysical;

    basicVmx.AsUint64 = VmxCapability(IA32_VMX_BASIC);

    //
    // Activate 13 bit (VMX enable)
    //
    cr4.u.raw = __readcr4();
    cr4.u.f.vmxe = 1;
    __writecr4(cr4.u.raw);


    //
    // Write revision identifier in VMXON region
    //
    *(UINT32*)vmxOn = basicVmx.Bits.revisionId;

    //
    // Activate virtualization
    //
    vmxOnPhysical = MmuGetPhysicalAddress(0, vmxOn);
    if (VmxOn((unsigned __int64*)&vmxOnPhysical) != 0)
    {
        //
        //Disable VMX bit
        //
        cr4.u.f.vmxe = 0;
        __writecr4(cr4.u.raw);
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

VOID
VmxpDisable(
    VOID
    )
{
    CR4_REGISTER cr4;

    VmxOff();

    cr4.u.raw = __readcr4();
    cr4.u.f.vmxe = 0;
    __writecr4(cr4.u.raw);
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

    hvm     = (PHVM)context;
    Vcpu = &hvm->VcpuArray[VcpuId];

    __cli();

    status = HvmpStartAsm( Vcpu );

    __sti();

    return status;
}

NTSTATUS __stdcall
HvmpStart(
    _In_ UINT_PTR  code,
    _In_ UINT_PTR  stack,
    _In_ UINT_PTR  flags,
    _In_ PHVM_VCPU Vcpu
    )
{
    NTSTATUS       status;
    FLAGS_REGISTER rflags;
    
    //
    // Check Vcpu
    //
    if(!Vcpu)
        return STATUS_UNSUCCESSFUL;

    //
    // Save host state
    //
    HvmpSaveHostState(&Vcpu->savedState);

    //
    // Enable VMX tech on this Vcpu
    //
    status = VmxpEnable(Vcpu->vmxOn);
    
    if(!NT_SUCCESS(status))
        return status;

    //
    // Clear and load VMCS
    //
    if(!VmcsClear(Vcpu->vmcs))
    {
        VmxpDisable();
        return STATUS_UNSUCCESSFUL;
    }

    if(!VmcsLoad(Vcpu->vmcs))
    {
        VmxpDisable();
        return STATUS_UNSUCCESSFUL;
    }

    //
    // HVM entry point config
    //
    rflags.u.raw  = flags;
    rflags.u.f.cf = 0;
    rflags.u.f.zf = 0;

    Vcpu->configure(Vcpu);

    VmxVmcsWritePlatform(HOST_RSP, Vcpu->rsp);
    VmxVmcsWritePlatform(HOST_RIP, (UINT_PTR)HvmpExitHandlerAsm);
    VmcsConfigureCommonEntry(code, stack, rflags);
   
    AtomicWrite(&Vcpu->launched, TRUE);
    
    return STATUS_SUCCESS;
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
        status = VMX_STATUS(VmxVmcsRead32(VM_INSTRUCTION_ERROR));
    }
    else {
        status = STATUS_UNSUCCESSFUL;
    }

    return status;
}

NTSTATUS __stdcall
HvmpStartFailure(
    _In_ PHVM_VCPU Vcpu,
    _In_ BOOLEAN   valid
    )
{
    NTSTATUS status;

    AtomicWrite(&Vcpu->launched, FALSE);
    status = HvmpFailure(Vcpu, valid);
    VmxpDisable();

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

    return HvmInternalCallAsm(HVM_INTERNAL_SERVICE_STOP, 0);
}

VOID
HvmpRestore(
    _In_ PHVM_VCPU Vcpu
    )
{
    CR4_REGISTER cr4 = { 0 };

    //
    //  TODO: restore FULL state: CR0, CR4, fsBase, tr, sysenterCs ?
    //

    __writeds( Vcpu->savedState.ds.u.raw );
    __writees( Vcpu->savedState.es.u.raw );
    __writefs( Vcpu->savedState.fs.u.raw );
#ifndef _WIN64
    __writess( Vcpu->savedState.ss.u.raw );
    __writegs( Vcpu->savedState.gs.u.raw );
#endif

    lgdt( Vcpu->savedState.gdt.base, Vcpu->savedState.gdt.limit );
    lidt( Vcpu->savedState.idt.base, Vcpu->savedState.idt.limit );

    //
    //  Disable TSD monitoring.
    //
    cr4.u.raw = VmxVmcsReadPlatform( GUEST_CR4 );
    cr4.u.f.tsd = 0;
    __writecr4( cr4.u.raw );

    //
    //  To allow the stop routine being invoked in any process context the CR3
    //  should be set to the guest CR3.
    //
    __writecr3( VmxVmcsReadPlatform( GUEST_CR3 ) );
}

VOID
HvmpStop(
    _In_ PHVM_VCPU Vcpu,
    _In_ PREGISTERS regs
    )
{
#ifdef _WIN64
    IRET_FRAME iret;

    iret.rip = regs->rip;
    iret.cs = Vcpu->savedState.cs.u.raw;
    iret.rflags = regs->rflags.u.raw;
    iret.rsp = regs->rsp;
    iret.ss = Vcpu->savedState.ss.u.raw;
#else

    PIRET_FRAME iretx86;

    iretx86 = (PIRET_FRAME) 
        ((PUINT_PTR) VmxVmcsReadPlatform( GUEST_RSP )
                     - (sizeof( IRET_FRAME ) / sizeof( UINT_PTR )));

    iretx86->rip = regs->rip;
    iretx86->cs = Vcpu->savedState.cs.u.raw;
    iretx86->rflags = regs->rflags.u.raw;
#endif
    
    //
    //  Restore segments, IDT and GDT from the saved state.
    //
    HvmpRestore( Vcpu );

    //
    //  This deactivates VMX operation in the processor.
    //
    VmxpDisable();

    //
    //  Restore registers and return from the interrupt.
    //
#ifdef _WIN64
    HvmpStopAsm((UINT_PTR)&iret, (UINT_PTR) regs );
#else
    HvmpStopAsm( (UINT_PTR)&iretx86, (UINT_PTR) regs );
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
