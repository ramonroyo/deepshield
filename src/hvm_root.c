#include "hvm.h"
#include "instr.h"
#include "vmx.h"
#include "smp.h"

extern PHVM gHvm;

PHVM_CORE ROOT_MODE_API
HvmCoreCurrent(
    VOID
)
{
    /*
    UINT32   i;
    UINT_PTR sp;

    sp = __readsp();

    for (i = 0; i < SmpNumberOfCores(); i++)
    {
        PHVM_CORE hvmCore = &gHvm.cores[i];

        UINT_PTR bottom = (UINT_PTR)hvmCore->stack;
        UINT_PTR top    = (UINT_PTR)hvmCore->rsp;

        if(bottom < sp && sp < top)
            return hvmCore;
    }

    return 0;
    */
    return &gHvm->cores[SmpCurrentCore()];
}

PREGISTERS ROOT_MODE_API
HvmCoreRegisters(
    _In_ PHVM_CORE core
)
{
    if(!core)
        return 0;

    return (PREGISTERS)(core->rsp - sizeof(REGISTERS));
}


PVOID ROOT_MODE_API
HvmCoreLocalContext(
    _In_ PHVM_CORE core
)
{
    if(!core)
        return 0;

    return core->localContext;
}

PVOID ROOT_MODE_API
HvmCoreGlobalContext(
    _In_ PHVM_CORE core
)
{
    if(!core)
        return 0;

    return core->hvm->globalContext;
}

UINT32 ROOT_MODE_API
HvmCoreIndex(
    _In_ PHVM_CORE core
)
{
    if (!core)
        return (UINT32)-1;

    return core->index;
}

PHVM ROOT_MODE_API
HvmCoreHvm(
    _In_ PHVM_CORE core
)
{
    if(!core)
        return 0;

    return core->hvm;
}

PHOST_SAVED_STATE ROOT_MODE_API
HvmCoreSavedState(
    _In_ PHVM_CORE core
)
{
    if (!core)
        return 0;

    return &core->savedState;
}

BOOLEAN ROOT_MODE_API
HvmCoreHandleCommonExits(
    _In_ UINT32     exitReason,
    _In_ PHVM_CORE  core,
    _In_ PREGISTERS regs
)
{
    UNREFERENCED_PARAMETER(core);

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

        //
        // Virtualization instructions
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
            regs->rflags.u.f.cf = 1;
            InstrRipAdvance(regs);
            return TRUE;
        }

        //
        // Routine VM exits
        //
        case EXIT_REASON_CR_ACCESS:
        {
            EXIT_QUALIFICATION_CR cr;
            UINT_PTR              gpr;

            cr.u.raw = VmxVmcsReadPlatform(EXIT_QUALIFICATION);
            gpr = *RegsGpr(regs, (UINT8)cr.u.f.gpr);

            InstrCrEmulate(cr.u.raw, regs);

            //
            // Make host exit CR3 follow current CR3 in guest
            //
            VmxVmcsWritePlatform(HOST_CR3, gpr);

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
        case EXIT_REASON_CPUID:
        {
            InstrCpuidEmulate(regs);
            InstrRipAdvance(regs);
            return TRUE;
        }
    }

    return FALSE;
}
