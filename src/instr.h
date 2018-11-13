#ifndef __INSTR_H__
#define __INSTR_H__

#include "vmx.h"
#include "hvm.h"

#define LOW32(X)  ((UINT32) ((UINT64)(X))       )
#define HIGH32(X) ((UINT32)(((UINT64)(X)) >> 32))

PUINTN
LookupGpr(
    _In_ PGP_REGISTERS Registers,
    _In_ UINT32     gpr
);

VOID
InstrInvdEmulate(
    _In_ PGP_REGISTERS Registers
);

BOOLEAN
InstrXsetbvEmulate(
    _In_ PGP_REGISTERS Registers
);

VOID
InstrInvVpidEmulate(
    _In_ PGP_REGISTERS Registers
);

VOID
InstrCpuidEmulate(
    _In_ PGP_REGISTERS Registers
);

VOID
InstrMsrReadEmulate(
    _In_ PGP_REGISTERS Registers
);

VOID
InstrMsrWriteEmulate(
    _In_ PGP_REGISTERS Registers
);

VOID
InstrInvlpgEmulate(
    _In_ UINTN exitQualification
);

VOID
InstrCr4Emulate(
    _In_ EXIT_QUALIFICATION_CR Qualification,
    _Inout_ PGP_REGISTERS Registers,
    _Inout_ PHVM_VCPU Vcpu
    )
    ;
VOID
InstrCr3Emulate(
    _In_ EXIT_QUALIFICATION_CR Qualification,
    _Inout_ PGP_REGISTERS Registers
);

/*
//
// Need to be refinated further, as DR7 is to be read and written from VMCS
//
VOID
InstrDrEmulate(
    _In_ UINTN   exitQualification,
    _In_ PGP_REGISTERS Registers
);
*/

VOID
InstrIoEmulate(
    _In_ UINTN   exitQualification,
    _In_ PGP_REGISTERS Registers
);

VOID
InstrRipAdvance(
    _In_ PGP_REGISTERS Registers
);

#endif
