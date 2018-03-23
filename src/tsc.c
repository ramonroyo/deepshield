#include "wdk7.h"
#include "tsc.h"
#include "context.h"
#include "instr.h"
#include "vmx.h"
#include "mmu.h"
#include "smp.h"
#include "mem.h"
#include "x86.h"

BOOLEAN
IsFreeSlot(
    _In_ PTSC_ENTRY Entry
)
{
    return Entry->Before.Address == 0;
}

PTSC_ENTRY GetFreeSlot(_In_ PTSC_ENTRY Head) {
    PTSC_ENTRY Hit = NULL;

    for (UINT32 i = 0; i < MAX_TSC_HITS; i++) {
         Hit = &Head[i];

        if ( Hit->Before.Address == 0 ) {
            return Hit;
        }
    }

    return NULL;
}

VOID
RdtscEmulate(
    _In_ PLOCAL_CONTEXT Local,
	_In_ PREGISTERS Regs
)
{
    UNREFERENCED_PARAMETER(Local);

    ULARGE_INTEGER TimeStamp = { 0 };
    TimeStamp.QuadPart = __readmsr(IA32_TSC);

    Regs->rdx = TimeStamp.HighPart;
    Regs->rax = TimeStamp.LowPart;

/*
    PTSC_ENTRY New = GetFreeSlot(Local->TscHits);

    if ( !New )
        FlushOldHits(Local->TscHits);
    }
*/

    InstrRipAdvance(Regs);
}

VOID
RdtscpEmulate(
    _In_ PLOCAL_CONTEXT Local,
	_In_ PREGISTERS Regs
)
{
    UNREFERENCED_PARAMETER(Local);

	UINT64 Processor = __readmsr(IA32_TSC_AUX);

    Regs->rcx = Processor;

    // Local->TscHits

    ULARGE_INTEGER TimeStamp = { 0 };
    TimeStamp.QuadPart = __readmsr(IA32_TSC);

    Regs->rdx = TimeStamp.HighPart;
    Regs->rax = TimeStamp.LowPart;
    Regs->rcx = Processor;

    InstrRipAdvance(Regs);
}

VOID
EnableUserTimeStamp(
    VOID
    )
{
    CR4_REGISTER cr4 = { 0 };
    cr4.u.raw = __readcr4();

    // disable TimeStamp when requested from user-mode
    cr4.u.f.tsd = 1;

    __writecr4(cr4.u.raw);
}

VOID
DisableUserTimeStamp(
    VOID
    )
{
    CR4_REGISTER cr4 = { 0 };

    cr4.u.raw = __readcr4();

    // disable TimeStamp when requested from user-mode
    cr4.u.f.tsd = 0;

    __writecr4(cr4.u.raw);
}
