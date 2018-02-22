#include "hvm.h"
#include "instr.h"
#include "vmx.h"

#define HVM_CALL_MAGIC              0xCAFEBABE
#define HVM_INTERNAL_SERVICE_STOP   0

VOID
HvmpExitEventLogVmcsInfo(
    _In_ PEXIT_INFO info
)
{
    info->exitReason                  = VmxVmcsRead32(EXIT_REASON);
    info->exitQualification           = VmxVmcsReadPlatform(EXIT_QUALIFICATION);
    info->guestLinearAddress          = VmxVmcsReadPlatform(GUEST_LINEAR_ADDRESS);
    info->guestPhyscalAddress         = VmxVmcsRead64(GUEST_PHYSICAL_ADDRESS);
    info->exitInterruptionInformation = VmxVmcsRead32(EXIT_INTERRUPTION_INFORMATION);
    info->exitInterruptionErrorCode   = VmxVmcsRead32(EXIT_INTERRUPTION_ERRORCODE);
    info->idtVectoringInformation     = VmxVmcsRead32(IDT_VECTORING_INFORMATION);
    info->idtVectoringErrorCode       = VmxVmcsRead32(IDT_VECTORING_ERRORCODE);
    info->exitInstructionLength       = VmxVmcsRead32(EXIT_INSTRUCTION_LENGTH);
    info->exitInstructionInformation  = VmxVmcsRead32(EXIT_INSTRUCTION_INFORMATION);
    info->ioRcx                       = VmxVmcsReadPlatform(IO_RCX);
    info->ioRsi                       = VmxVmcsReadPlatform(IO_RSI);
    info->ioRdi                       = VmxVmcsReadPlatform(IO_RDI);
    info->ioRip                       = VmxVmcsReadPlatform(IO_RIP);
    info->vmInstructionError          = VmxVmcsReadPlatform(VM_INSTRUCTION_ERROR);
}

VOID
HvmpExitEventLog(
    _In_ PHVM_CORE  core,
    _In_ PREGISTERS regs
)
{
    UINT_PTR index = 0;

    //
    // Calculate new index
    //
    index = core->loggedEvents.numberOfEvents % MAX_NUMBER_OF_LOGGED_EXIT_EVENTS;
    
    //
    // Log exit data
    //
    HvmpExitEventLogVmcsInfo(&core->loggedEvents.queue[index].info);
    core->loggedEvents.queue[index].regs = *regs;

    //
    // Increase number of events serviced
    //
    core->loggedEvents.numberOfEvents++;
}


extern VOID
HvmpStop(
    _In_ PHVM_CORE  core,
    _In_ PREGISTERS regs
);


VOID
HvmHandleService(
    _In_ PHVM_CORE  core,
    _In_ PREGISTERS regs
)
{
    UINT_PTR service;

    service = regs->rcx;

    //
    // Advance RIP for the call
    //
    InstrRipAdvance(regs);

    //
    // Perform service
    //
    switch (service)
    {
        case HVM_INTERNAL_SERVICE_STOP:
        {
            regs->rax = (UINT_PTR)STATUS_SUCCESS;

            HvmpStop(core, regs);

            //
            // Never reached
            //
            break;
        }

        default:
        {
            regs->rax = (UINT32)STATUS_UNSUCCESSFUL;

            regs->rflags.u.f.cf = 1;

            break;
        }
    }
}

VOID __stdcall
HvmpExitHandler(
    _In_ PHVM_CORE  core,
    _In_ PREGISTERS regs
)
{
    UINT32 exitReason;

    //
    // Gather minimal exit information
    //
    exitReason = VmxVmcsRead32(EXIT_REASON);

    //
    // Correct guest regs
    //
    regs->rsp          = VmxVmcsReadPlatform(GUEST_RSP);
    regs->rip          = VmxVmcsReadPlatform(GUEST_RIP);
    regs->rflags.u.raw = VmxVmcsReadPlatform(GUEST_RFLAGS);

    //
    // Log event
    //
    HvmpExitEventLog(core, regs);

    //
    // Processing
    //
    if(exitReason == EXIT_REASON_VMCALL && regs->rax == HVM_CALL_MAGIC)
    {
        //
        // Invoke Internal Services Handler
        //
        HvmHandleService(core, regs);
    }
    else
    {
        //
        // Invoke HVM Handler
        //
        core->handler(exitReason, core, regs);
    }

    //
    // Correct guest regs after processing
    //
    VmxVmcsWritePlatform(GUEST_RSP,    regs->rsp);
    VmxVmcsWritePlatform(GUEST_RIP,    regs->rip);
    VmxVmcsWritePlatform(GUEST_RFLAGS, regs->rflags.u.raw);
}
