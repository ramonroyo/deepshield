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
    info->exitReason                  = VmRead32(EXIT_REASON);
    info->exitQualification           = VmReadN(EXIT_QUALIFICATION);
    info->guestLinearAddress          = VmReadN(GUEST_LINEAR_ADDRESS);
    info->guestPhysicalAddress        = VmRead64(GUEST_PHYSICAL_ADDRESS);
    info->exitInterruptionInformation = VmRead32(EXIT_INTERRUPTION_INFORMATION);
    info->exitInterruptionErrorCode   = VmRead32(EXIT_INTERRUPTION_ERRORCODE);
    info->idtVectoringInformation     = VmRead32(IDT_VECTORING_INFORMATION);
    info->idtVectoringErrorCode       = VmRead32(IDT_VECTORING_ERRORCODE);
    info->exitInstructionLength       = VmRead32(EXIT_INSTRUCTION_LENGTH);
    info->exitInstructionInformation  = VmRead32(EXIT_INSTRUCTION_INFORMATION);
    info->ioRcx                       = VmReadN(IO_RCX);
    info->ioRsi                       = VmReadN(IO_RSI);
    info->ioRdi                       = VmReadN(IO_RDI);
    info->ioRip                       = VmReadN(IO_RIP);
    info->vmInstructionError          = VmReadN(VM_INSTRUCTION_ERROR);
}

VOID
HvmpExitEventLog(
    _In_ PHVM_VCPU  Vcpu,
    _In_ PGP_REGISTERS Registers
    )
{
    UINTN Index = 0;

    Index = Vcpu->LoggedEvents.Count % MAX_NUMBER_OF_LOGGED_EXIT_EVENTS;

    HvmpExitEventLogVmcsInfo( &Vcpu->LoggedEvents.Queue[Index].info);

    Vcpu->LoggedEvents.Queue[Index].Registers = *Registers;
    Vcpu->LoggedEvents.Count++;
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
    UINTN service;

    service = Registers->Rcx;

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
            Registers->Rax = (UINTN)STATUS_SUCCESS;

            HvmpStop( Vcpu, Registers );
            break;
        }

        default:
        {
            Registers->Rax = (UINT32)STATUS_UNSUCCESSFUL;
            Registers->Rflags.Bits.Cf = 1;

            break;
        }
    }
}

VOID __stdcall
HvmpExitHandler(
    _In_ PHVM_VCPU Vcpu,
    _In_ PGP_REGISTERS Registers
    )
{
    VM_INFO_BASIC InfoBasic;

    NT_ASSERT( &Vcpu->GuestRegisters == Registers );

    InfoBasic.AsUint32 = VmRead32( EXIT_REASON );

    //
    //  Complete missing guest registers.
    //

    Registers->Rsp = VmReadN( GUEST_RSP );
    Registers->Rip = VmReadN( GUEST_RIP );
    Registers->Rflags.AsUintN = VmReadN( GUEST_RFLAGS );

    HvmpExitEventLog( Vcpu, Registers );

    if (InfoBasic.Bits.Reason == EXIT_REASON_VMCALL
            && Registers->Rax == HVM_CALL_MAGIC) {

        //
        //  Invoke Internal Services Handler.
        //
        HvmHandleService( Vcpu, Registers );
    }
    else {

        //
        //  Invoke HVM Handler.
        //
        Vcpu->ExitHandler( InfoBasic.Bits.Reason, Vcpu, Registers );
    }

    //
    //  Update guest registers after processing.
    //

    VmWriteN( GUEST_RSP, Registers->Rsp );
    VmWriteN( GUEST_RIP, Registers->Rip );
    VmWriteN( GUEST_RFLAGS, Registers->Rflags.AsUintN );
}