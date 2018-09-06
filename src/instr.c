#include "wdk7.h"
#include "hvm.h"
#include "vmx.h"
#include "x86.h"
#include "mmu.h"

#define LOW32(X)  ((UINT32) ((UINT64)(X))       )
#define HIGH32(X) ((UINT32)(((UINT64)(X)) >> 32))

PUINT_PTR
LookupGpr(
    _In_ PREGISTERS regs,
    _In_ UINT8      gpr
)
{
#ifndef _WIN64
    gpr = 7 - gpr;
#endif

    //
    //  TODO: use new REGISTERS for 32-bits build.
    //
    return (PUINT_PTR)regs + gpr;
}

VOID
InstrInvdEmulate(
    _In_ PREGISTERS regs
    )
{
    UNREFERENCED_PARAMETER(regs);
    __wbinvd();
}

VOID
InstrXsetbvEmulate(
    _In_ PREGISTERS regs
    )
{
#ifdef _WIN64
    __xsetbv((UINT32)regs->rcx, regs->rdx << 32 | (UINT32)regs->rax);
#else
    UNREFERENCED_PARAMETER(regs);
#endif
}

VOID
InstrInvVpidEmulate(
    _In_ PREGISTERS regs
    )
{
    VPID_CTX ctx = { 0 };

    UNREFERENCED_PARAMETER(regs);
    __invvpid( INV_ALL_CONTEXTS, &ctx );
}

VOID
InstrCpuidEmulate(
    _In_ PREGISTERS regs
)
{
    int cpuRegs[4] = { 0 };

    //__cpuid(cpuRegs, (int)regs->rax);
    __cpuidex( (int*)cpuRegs, (int) regs->rax, (int) regs->rcx );

    regs->rax = (UINT_PTR) cpuRegs[0];
    regs->rbx = (UINT_PTR) cpuRegs[1];
    regs->rcx = (UINT_PTR) cpuRegs[2];
    regs->rdx = (UINT_PTR) cpuRegs[3];
}

VOID
InstrMsrReadEmulate(
    _In_ PREGISTERS regs
)
{
    UINT64 value;

    value = __readmsr(LOW32(regs->rcx));

    regs->rax = (UINT_PTR) LOW32(value);
    regs->rdx = (UINT_PTR) HIGH32(value);
}

VOID
InstrMsrWriteEmulate(
    _In_ PREGISTERS regs
)
{
    UINT64 value;

    value = (((UINT64)LOW32(regs->rdx)) << 32) | LOW32(regs->rax);

    __writemsr(LOW32(regs->rcx), value);
}

VOID
InstrInvlpgEmulate(
    _In_ UINT_PTR exitQualification
)
{
    __invlpg((PVOID)exitQualification);
}

VOID
InstrCrEmulate(
    _In_ UINT_PTR   exitQualification,
    _In_ PREGISTERS regs
)
{
    PUINT_PTR             gpr;
    EXIT_QUALIFICATION_CR data;

    data.u.raw = exitQualification;

    gpr = LookupGpr(regs, (UINT8)data.u.cr.moveGpr);

    if (data.u.cr.accessType == CR_ACCESS_TYPE_MOV_FROM_CR)
    {
        switch(data.u.cr.number)
        {
            case 3: *gpr = VmxVmcsReadPlatform(GUEST_CR3); break;
            case 8: *gpr = readcr(data.u.cr.number);       break;
        }
    }
    else if (data.u.cr.accessType == CR_ACCESS_TYPE_MOV_TO_CR)
    {
        switch(data.u.cr.number)
        {
            case 3: VmxVmcsWritePlatform(GUEST_CR3, *gpr); break;
            case 8: writecr(data.u.cr.number, *gpr);       break;
        }
    }

#ifndef _WIN64
    if ((data.u.cr.accessType == CR_ACCESS_TYPE_MOV_TO_CR) &&
        (data.u.cr.number == 3)                            &&
        (VmxVmcsReadPlatform(GUEST_CR4) & CR4_PAE_ENABLED)
    )
    {
        VMX_PROC_SECONDARY_CTLS ctls;

        ctls.u.raw = VmxVmcsRead32(VM_EXEC_CONTROLS_PROC_SECONDARY);

        if(ctls.u.f.enableEpt)
        {
            PHYSICAL_ADDRESS cr3;
            PVOID            page;
            PUINT64          pdptes;

            cr3.HighPart = 0;
            cr3.LowPart  = *gpr;

            page = MmuMap(cr3);

            pdptes = (PUINT64)((UINT_PTR)page + ((*gpr) & 0xFF0));

            VmxVmcsWrite64(GUEST_PDPTE_0, pdptes[0]);
            VmxVmcsWrite64(GUEST_PDPTE_1, pdptes[1]);
            VmxVmcsWrite64(GUEST_PDPTE_2, pdptes[2]);
            VmxVmcsWrite64(GUEST_PDPTE_3, pdptes[3]);

            MmuUnmap(page);
        }
    }
#endif
}

/*
VOID
InstrDrEmulate(
    _In_ UINT_PTR   exitQualification,
    _In_ PREGISTERS regs
)
{
    PUINT_PTR             gpr;
    EXIT_QUALIFICATION_DR data;

    data.u.raw = exitQualification;

    gpr = LookupGpr(regs, (UINT8)data.u.f.gpr);

    if (data.u.f.accessType == DR_ACCESS_TYPE_MOV_FROM_DR)
    {
        *gpr = readdr(data.u.f.dr);
    }
    else if (data.u.f.accessType == DR_ACCESS_TYPE_MOV_TO_DR)
    {
        writedr(data.u.f.dr, *gpr);
    }
}
*/

VOID
InstrIoEmulate(
    _In_ UINT_PTR   exitQualification,
    _In_ PREGISTERS regs
)
{
    EXIT_QUALIFICATION_IO data;

    data.u.raw = exitQualification;

    if (data.u.f.direction)
    {
        if (data.u.f.isStringInstruction)
        {
            switch (data.u.f.size)
            {
            case 1: __inbytestring ((USHORT)data.u.f.port, (PUCHAR) regs->rdi, (ULONG)(data.u.f.hasRepPrefix ? regs->rcx : 1)); break;
            case 2: __inwordstring ((USHORT)data.u.f.port, (PUSHORT)regs->rdi, (ULONG)(data.u.f.hasRepPrefix ? regs->rcx : 1)); break;
            case 4: __indwordstring((USHORT)data.u.f.port, (PULONG)regs->rdi,  (ULONG)(data.u.f.hasRepPrefix ? regs->rcx : 1)); break;
            }
        }
        else
        {
            switch (data.u.f.size)
            {
#ifndef _WIN64
            case 1: regs->rax = (regs->rax & 0xFFFFFF00) | __inbyte (data.u.f.port);
            case 2: regs->rax = (regs->rax & 0xFFFF0000) | __inword (data.u.f.port);
            case 4: regs->rax =                            __indword(data.u.f.port);
#else
            case 1: regs->rax = (regs->rax & 0xFFFFFFFFFFFFFF00) | __inbyte ((USHORT)data.u.f.port);
            case 2: regs->rax = (regs->rax & 0xFFFFFFFFFFFF0000) | __inword ((USHORT)data.u.f.port);
            case 4: regs->rax = (regs->rax & 0xFFFFFFFF00000000) | __indword((USHORT)data.u.f.port);
#endif
            }
        }
    }
    else
    {
        if (data.u.f.isStringInstruction)
        {
            switch (data.u.f.size)
            {
            case 1: __outbytestring ((USHORT)data.u.f.port, (PUCHAR) regs->rsi, (ULONG)(data.u.f.hasRepPrefix ? regs->rcx : 1)); break;
            case 2: __outwordstring ((USHORT)data.u.f.port, (PUSHORT)regs->rsi, (ULONG)(data.u.f.hasRepPrefix ? regs->rcx : 1)); break;
            case 4: __outdwordstring((USHORT)data.u.f.port, (PULONG) regs->rsi, (ULONG)(data.u.f.hasRepPrefix ? regs->rcx : 1)); break;
            }
        }
        else
        {
            switch (data.u.f.size)
            {
            case 1: __outbyte ((USHORT)data.u.f.port, (UCHAR) regs->rax);  break;
            case 2: __outword ((USHORT)data.u.f.port, (USHORT)regs->rax); break;
            case 4: __outdword((USHORT)data.u.f.port, (ULONG) regs->rax); break;
            }
        }
    }
}

BOOLEAN 
IsSingleStep(
    _In_ PREGISTERS Regs
    )
{
    return Regs->rflags.u.f.tf == TRUE;
}

VOID 
InjectInt1(
    _In_ PREGISTERS Regs
    )
{
    INTERRUPT_INFORMATION Int1 = { 0 };

    Int1.u.f.valid = 1;
    Int1.u.f.vector = 1;
    Int1.u.f.type = INTERRUPT_TYPE_HARDWARE_EXCEPTION;

    //
    // Clear trap flag
    //
    Regs->rflags.u.f.tf = 0;

    VmxVmcsWrite32(VM_ENTRY_INTERRUPTION_INFORMATION, Int1.u.raw);
}

VOID
InstrRipAdvance(
    _In_ PREGISTERS Regs
    )
{
    UINT32 InstructionLength;

    InstructionLength = VmxVmcsRead32(EXIT_INSTRUCTION_LENGTH);

    Regs->rip += InstructionLength;

    //
    //  Single step emulation (has TF enabled).
    //
    if (IsSingleStep(Regs))
    {
        //
        //  Injects an int1 and clears trap flag.
        //
        InjectInt1(Regs);
    }
}
