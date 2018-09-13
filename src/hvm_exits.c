#include "wdk7.h"
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
    info->exitReason                  = VmxRead32(EXIT_REASON);
    info->exitQualification           = VmxReadPlatform(EXIT_QUALIFICATION);
    info->guestLinearAddress          = VmxReadPlatform(GUEST_LINEAR_ADDRESS);
    info->guestPhyscalAddress         = VmxRead64(GUEST_PHYSICAL_ADDRESS);
    info->exitInterruptionInformation = VmxRead32(EXIT_INTERRUPTION_INFORMATION);
    info->exitInterruptionErrorCode   = VmxRead32(EXIT_INTERRUPTION_ERRORCODE);
    info->idtVectoringInformation     = VmxRead32(IDT_VECTORING_INFORMATION);
    info->idtVectoringErrorCode       = VmxRead32(IDT_VECTORING_ERRORCODE);
    info->exitInstructionLength       = VmxRead32(EXIT_INSTRUCTION_LENGTH);
    info->exitInstructionInformation  = VmxRead32(EXIT_INSTRUCTION_INFORMATION);
    info->ioRcx                       = VmxReadPlatform(IO_RCX);
    info->ioRsi                       = VmxReadPlatform(IO_RSI);
    info->ioRdi                       = VmxReadPlatform(IO_RDI);
    info->ioRip                       = VmxReadPlatform(IO_RIP);
    info->vmInstructionError          = VmxReadPlatform(VM_INSTRUCTION_ERROR);
}

VOID
HvmpExitEventLog(
    _In_ PHVM_VCPU  Vcpu,
    _In_ PGP_REGISTERS Registers
)
{
    UINT_PTR index = 0;

    //
    // Calculate new index
    //
    index = Vcpu->LoggedEvents.numberOfEvents % MAX_NUMBER_OF_LOGGED_EXIT_EVENTS;
    
    //
    // Log exit data
    //
    HvmpExitEventLogVmcsInfo(&Vcpu->LoggedEvents.queue[index].info);

    Vcpu->LoggedEvents.queue[index].Registers = *Registers;
    Vcpu->LoggedEvents.numberOfEvents++;
}


extern VOID
HvmpStop(
    _In_ PHVM_VCPU  Vcpu,
    _In_ PGP_REGISTERS Registers
);


VOID
HvmHandleService(
    _In_ PHVM_VCPU  Vcpu,
    _In_ PGP_REGISTERS Registers
)
{
    UINT_PTR service;

    service = Registers->rcx;

    //
    // Advance RIP for the call
    //
    InstrRipAdvance( Registers );

    //
    // Perform service
    //
    switch (service)
    {
        case HVM_INTERNAL_SERVICE_STOP:
        {
            Registers->rax = (UINT_PTR)STATUS_SUCCESS;

            HvmpStop( Vcpu, Registers );

            //
            // Never reached
            //
            break;
        }

        default:
        {
            Registers->rax = (UINT32)STATUS_UNSUCCESSFUL;
            Registers->rflags.u.f.cf = 1;

            break;
        }
    }
}

//
// There are some assumptions when HOST state is loaded, most important are described at 
// Intel® 64 and IA-32 Architectures Software Developer’s Manual, Volume 3 27.5 (Loading Host State)
// 
VOID __stdcall
HvmpExitHandler(
    _In_ PHVM_VCPU Vcpu,
    _In_ PGP_REGISTERS Registers
    )
{
    UINT32 exitReason;

    NT_ASSERT( &Vcpu->GuestRegisters == Registers );

    //
    // Gather minimal exit information
    //
    exitReason = VmxRead32( EXIT_REASON );

    //
    // Correct guest Registers
    //
    Registers->rsp          = VmxReadPlatform(GUEST_RSP);
    Registers->rip          = VmxReadPlatform(GUEST_RIP);
    Registers->rflags.u.raw = VmxReadPlatform(GUEST_RFLAGS);

    //
    // Log event
    //
    HvmpExitEventLog(Vcpu, Registers);

    //
    // Processing
    //
    if (exitReason == EXIT_REASON_VMCALL && Registers->rax == HVM_CALL_MAGIC) {
        //
        // Invoke Internal Services Handler
        //
        HvmHandleService( Vcpu, Registers );
    }
    else {
        //
        // Invoke HVM Handler
        //
        Vcpu->ExitHandler( exitReason, Vcpu, Registers );
    }

    //
    // Correct guest Registers after processing
    //
    VmxWritePlatform( GUEST_RSP,    Registers->rsp );
    VmxWritePlatform( GUEST_RIP,    Registers->rip );
    VmxWritePlatform( GUEST_RFLAGS, Registers->rflags.u.raw );
}
