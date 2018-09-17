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
HvmGetVcpuContext(
    _In_ PHVM_VCPU Vcpu
    )
{
    NT_ASSERT(Vcpu);
    return Vcpu->LocalContext;
}

PVOID ROOT_MODE_API
HvmGetHvmContext(
    _In_ PHVM_VCPU Vcpu
    )
{
    if (!Vcpu) {
        return 0;
    }

    return Vcpu->Hvm->HvmContext;
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
HvmGetVcpuHostState(
    _In_ PHVM_VCPU Vcpu
    )
{
    if (!Vcpu) {
        return 0;
    }

    return &Vcpu->HostState;
}

BOOLEAN ROOT_MODE_API
HvmVcpuCommonExitsHandler(
    _In_ UINT32 exitReason,
    _In_ PHVM_VCPU Vcpu,
    _In_ PGP_REGISTERS Registers
    )
{
    UNREFERENCED_PARAMETER(Vcpu);

    switch (exitReason)
    {
        case EXIT_REASON_INVD:
        {
            InstrInvdEmulate( Registers );
            InstrRipAdvance( Registers );
            return TRUE;
        }
#ifdef _WIN64
        case EXIT_REASON_XSETBV:
        {
            if (InstrXsetbvEmulate( Registers )) {
                InstrRipAdvance( Registers );
            }
            return TRUE;
        }
#endif

#ifdef HVM_EMULATE_INVVPID
        case EXIT_REASON_INVVPID:
        {
            InstrInvVpidEmulate( Registers );
            InstrRipAdvance( Registers );
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
            InjectUdException();

            Registers->Rflags.Bits.rf = 1;
            VmWriteN( GUEST_RFLAGS, Registers->Rflags.AsUintN );

            //InstrRipAdvance( Registers );
            return TRUE;
        }

        case EXIT_REASON_MSR_READ:
        {
            InstrMsrReadEmulate(Registers);
            InstrRipAdvance( Registers );
            return TRUE;
        }
        case EXIT_REASON_MSR_WRITE:
        {
            InstrMsrWriteEmulate( Registers );
            InstrRipAdvance( Registers );
            return TRUE;
        }
    }

    return FALSE;
}
