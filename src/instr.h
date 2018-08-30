#ifndef __INSTR_H__
#define __INSTR_H__

#include "hvm.h"

PUINT_PTR
LookupGpr(
    _In_ PREGISTERS regs,
    _In_ UINT8      gpr
);

VOID
InstrInvdEmulate(
    _In_ PREGISTERS regs
);

VOID
InstrXsetbvEmulate(
    _In_ PREGISTERS regs
);

VOID
InstrCpuidEmulate(
    _In_ PREGISTERS regs
);

VOID
InstrMsrReadEmulate(
    _In_ PREGISTERS regs
);

VOID
InstrMsrWriteEmulate(
    _In_ PREGISTERS regs
);

VOID
InstrInvlpgEmulate(
    _In_ UINT_PTR exitQualification
);

//
// Only works correctly for CR3 and CR8
// No support yet for CR0 and CR4
//
VOID
InstrCrEmulate(
    _In_ UINT_PTR   exitQualification,
    _In_ PREGISTERS regs
);

/*
//
// Need to be refinated further, as DR7 is to be read and written from VMCS
//
VOID
InstrDrEmulate(
    _In_ UINT_PTR   exitQualification,
    _In_ PREGISTERS regs
);
*/

/*
//
// Need to be refinated further, not tested and can be refatored the index and base parts to generic functions
//
VOID
InstrXdtEmulate(
    _In_ UINT_PTR   exitQualification,
    _In_ PREGISTERS regs
);

VOID
InstrXtrEmulate(
    _In_ UINT_PTR   exitQualification,
    _In_ PREGISTERS regs
);
*/

VOID
InstrIoEmulate(
    _In_ UINT_PTR   exitQualification,
    _In_ PREGISTERS regs
);

VOID
InterruptEmulate(
    _In_ UINT32   interruptInformation,
    _In_ UINT32   errorCode,
    _In_ UINT_PTR exitQualification
);

VOID
InstrRipAdvance(
    _In_ PREGISTERS regs
);

#endif
