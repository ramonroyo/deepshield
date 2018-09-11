#include "wdk7.h"
#include "hvm.h"
#include "instr.h"
#include "vmx.h"
#include "smp.h"

extern PHVM gHvm;

PHVM_VCPU ROOT_MODE_API
HvmGetCurrentVcpu(
    VOID
    )
{
    return &gHvm->VcpuArray[SmpGetCurrentProcessor()];
}

PVOID ROOT_MODE_API
HvmGetVcpuLocalContext(
    _In_ PHVM_VCPU Vcpu
    )
{
    if (!Vcpu) {
        return 0;
    }

    return Vcpu->LocalContext;
}

PVOID ROOT_MODE_API
HvmGetVcpuGlobalContext(
    _In_ PHVM_VCPU Vcpu
    )
{
    if (!Vcpu) {
        return 0;
    }

    return Vcpu->Hvm->globalContext;
}

UINT32 ROOT_MODE_API
HvmGetVcpuId(
    _In_ PHVM_VCPU Vcpu
   )
{
    if (!Vcpu) {
        return (UINT32)-1;
    }

    return Vcpu->Index;
}

PHVM ROOT_MODE_API
HvmGetVcpuHvm(
    _In_ PHVM_VCPU Vcpu
    )
{
    if (!Vcpu) {
        return 0;
    }

    return Vcpu->Hvm;
}

PHOST_SAVED_STATE ROOT_MODE_API
HvmGetVcpuSavedState(
    _In_ PHVM_VCPU Vcpu
    )
{
    if (!Vcpu) {
        return 0;
    }

    return &Vcpu->SavedState;
}

BOOLEAN ROOT_MODE_API
HvmVcpuCommonExitsHandler(
    _In_ UINT32 exitReason,
    _In_ PHVM_VCPU Vcpu,
    _In_ PREGISTERS regs
    )
{
    UNREFERENCED_PARAMETER(Vcpu);

    switch (exitReason)
    {
        case EXIT_REASON_INVD:
        {
            InstrInvdEmulate(regs);
            InstrRipAdvance(regs);
            return TRUE;
        }
#ifdef _WIN64
        case EXIT_REASON_XSETBV:
        {
            InstrXsetbvEmulate(regs);
            InstrRipAdvance(regs);
            return TRUE;
        }
#endif

#ifdef HVM_EMULATE_INVVPID
        case EXIT_REASON_INVVPID:
        {
            InstrInvVpidEmulate( regs );
            InstrRipAdvance( regs );
            return TRUE;
        }
#endif

        //
        //  Virtualization instructions
        //
        case EXIT_REASON_VMXON:
        case EXIT_REASON_VMXOFF:
        case EXIT_REASON_VMCLEAR:
        case EXIT_REASON_VMRESUME:
        case EXIT_REASON_VMLAUNCH:
        case EXIT_REASON_VMPTRLD:
        case EXIT_REASON_VMPTRST:
        case EXIT_REASON_VMREAD:
        case EXIT_REASON_VMWRITE:
        case EXIT_REASON_INVEPT:
        case EXIT_REASON_INVVPID:
        {
            InjectUndefinedOpcodeException();

            regs->rflags.u.f.rf = 1;
            VmxVmcsWritePlatform( GUEST_RFLAGS, regs->rflags.u.raw );

            InstrRipAdvance(regs);
            return TRUE;
        }

        case EXIT_REASON_MSR_READ:
        {
            InstrMsrReadEmulate(regs);
            InstrRipAdvance(regs);
            return TRUE;
        }
        case EXIT_REASON_MSR_WRITE:
        {
            InstrMsrWriteEmulate(regs);
            InstrRipAdvance(regs);
            return TRUE;
        }
    }

    return FALSE;
}
