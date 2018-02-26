#include "wdk7.h"
#include "hvm.h"
#include "vmx.h"
#include "x86.h"
#include "mmu.h"

#define LOW32(X)  ((UINT32) ((UINT64)(X))       )
#define HIGH32(X) ((UINT32)(((UINT64)(X)) >> 32))

PUINT_PTR
RegsGpr(
    _In_ PREGISTERS regs,
    _In_ UINT8      gpr
)
{
#ifdef _WIN64
    gpr = 15 - gpr;
#else
    gpr = 7 - gpr;
#endif

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
    __xsetbv((UINT32)regs->rcx, regs->rdx << 32 | regs->rax);
#else
    UNREFERENCED_PARAMETER(regs);
#endif
}

VOID
InstrCpuidEmulate(
    _In_ PREGISTERS regs
)
{
    int cpuRegs[4] = { 0 };

    __cpuid(cpuRegs, (int)regs->rax);

#ifndef _WIN64
    regs->rax = cpuRegs[0];
    regs->rbx = cpuRegs[1];
    regs->rcx = cpuRegs[2];
    regs->rdx = cpuRegs[3];
#else
    regs->rax = (regs->rax & 0xFFFFFFFF00000000) | cpuRegs[0];
    regs->rbx = (regs->rbx & 0xFFFFFFFF00000000) | cpuRegs[1];
    regs->rcx = (regs->rcx & 0xFFFFFFFF00000000) | cpuRegs[2];
    regs->rdx = (regs->rdx & 0xFFFFFFFF00000000) | cpuRegs[3];
#endif
}

VOID
InstrMsrReadEmulate(
    _In_ PREGISTERS regs
)
{
    UINT64 value;

    value = __readmsr(LOW32(regs->rcx));

#ifndef _WIN64
    regs->rax = LOW32(value);
    regs->rdx = HIGH32(value);
#else
    regs->rax = (regs->rax & 0xFFFFFFFF00000000) | LOW32(value);
    regs->rdx = (regs->rdx & 0xFFFFFFFF00000000) | HIGH32(value);
#endif
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

    gpr = RegsGpr(regs, (UINT8)data.u.f.gpr);

    if (data.u.f.accessType == CR_ACCESS_TYPE_MOV_FROM_CR)
    {
        switch(data.u.f.cr)
        {
            case 3: *gpr = VmxVmcsReadPlatform(GUEST_CR3); break;
            case 8: *gpr = readcr(data.u.f.cr);            break;
        }
    }
    else if (data.u.f.accessType == CR_ACCESS_TYPE_MOV_TO_CR)
    {
        switch(data.u.f.cr)
        {
            case 3: VmxVmcsWritePlatform(GUEST_CR3, *gpr); break;
            case 8: writecr(data.u.f.cr, *gpr);            break;
        }
    }

#ifndef _WIN64
    if ((data.u.f.accessType == CR_ACCESS_TYPE_MOV_TO_CR) &&
        (data.u.f.cr == 3)                                &&
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

    gpr = RegsGpr(regs, (UINT8)data.u.f.gpr);

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

/*
VOID
InstrXdtEmulate(
    _In_ UINT_PTR   exitQualification,
    _In_ PREGISTERS regs
)
{
    INSTRUCTION_INFORMATION_XDT instrInfo;

    UINT_PTR base         = 0;
    UINT_PTR index        = 0;
    UINT_PTR displacement = 0;
    UINT_PTR address      = 0;

    PUINT16   tableLimit;
    PUINT_PTR tableBase;

    instrInfo.u.raw = VmxVmcsRead32(EXIT_INSTRUCTION_INFORMATION);

    //
    // Base
    //
    if (!instrInfo.u.f.baseInvalid)
    {
        PUINT_PTR reg = RegsGpr(regs, (UINT8)instrInfo.u.f.base);
        base = *reg;
    }

    //
    // Index
    //
    if (!instrInfo.u.f.indexInvalid)
    {
        PUINT_PTR reg = RegsGpr(regs, (UINT8)instrInfo.u.f.index);
        index = *reg;
        switch (instrInfo.u.f.scaling)
        {
            case 0:             break;
            case 1: index *= 2; break;
            case 2: index *= 4; break;
            case 3: index *= 8; break;
            default: break;
        }
    }

    //
    // Displacement
    //
    displacement = exitQualification;

    //
    // Address
    //
    address = base + index + displacement;
    if (instrInfo.u.f.addressSize == 1)
    {
        //
        // 32-bit case
        //
        address &= MAXULONG;
    }

    //
    // Emulate
    //
    tableLimit = (PUINT16)   address;
    tableBase  = (PUINT_PTR)(address + sizeof(UINT16));

    switch (instrInfo.u.f.instruction)
    {
        case INSTRUCTION_INFORMATION_IDENTITY_SGDT:
        {
            *tableBase  = VmxVmcsReadPlatform(GUEST_GDTR_BASE);
            *tableLimit = (UINT16)VmxVmcsRead32(GUEST_GDTR_LIMIT);
            break;
        }
        case INSTRUCTION_INFORMATION_IDENTITY_SIDT:
        {
            *tableBase  = VmxVmcsReadPlatform(GUEST_IDTR_BASE);
            *tableLimit = (UINT16)VmxVmcsRead32(GUEST_IDTR_LIMIT);
            break;
        }
        case INSTRUCTION_INFORMATION_IDENTITY_LGDT:
        {
            VmxVmcsWritePlatform(GUEST_GDTR_BASE,  *tableBase);
            VmxVmcsWrite32(      GUEST_GDTR_LIMIT, *tableLimit);
            break;
        }
        case INSTRUCTION_INFORMATION_IDENTITY_LIDT:
        {
            VmxVmcsWritePlatform(GUEST_IDTR_BASE,  *tableBase);
            VmxVmcsWrite32(      GUEST_IDTR_LIMIT, *tableLimit);
            break;
        }
    }
}

VOID
InstrXtrEmulate(
    _In_ UINT_PTR   exitQualification,
    _In_ PREGISTERS regs
)
{
    INSTRUCTION_INFORMATION_XTR instrInfo;

    UINT_PTR base         = 0;
    UINT_PTR index        = 0;
    UINT_PTR displacement = 0;

    UINT_PTR address = 0;

    PUINT16 selector;

    instrInfo.u.raw = VmxVmcsRead32(EXIT_INSTRUCTION_INFORMATION);

    if (instrInfo.u.f.memReg)
    {
        //
        // Register access
        //
        PUINT_PTR reg = RegsGpr(regs, (UINT8)instrInfo.u.f.reg1);
        address = reg;
    }
    else
    {
        //
        // Memory access
        //

        //
        // Base
        //
        if (!instrInfo.u.f.baseInvalid)
        {
            PUINT_PTR reg = RegsGpr(regs, (UINT8)instrInfo.u.f.base);
            base = *reg;
        }

        //
        // Index
        //
        if (!instrInfo.u.f.indexInvalid)
        {
            PUINT_PTR reg = RegsGpr(regs, (UINT8)instrInfo.u.f.index);
            index = *reg;
            switch (instrInfo.u.f.scaling)
            {
                case 0:             break;
                case 1: index *= 2; break;
                case 2: index *= 4; break;
                case 3: index *= 8; break;
                default: break;
            }
        }

        //
        // Displacement
        //
        displacement = VmxVmcsReadPlatform(EXIT_QUALIFICATION);

        //
        // Final address
        //
        address = base + index + displacement;
    
        if (instrInfo.u.f.addressSize == 1)
        {
            //
            // 32-bit correction
            //
            address &= MAXULONG;
        }
    }

    //
    // Emulate
    //
    selector = (PUINT16)address;
    switch (instrInfo.u.f.instruction)
    {
        case INSTRUCTION_INFORMATION_IDENTITY_SLDT:
        {
            *selector = VmxVmcsRead16(GUEST_LDTR_SELECTOR);
            break;
        }
        case INSTRUCTION_INFORMATION_IDENTITY_STR:
        {
            *selector = VmxVmcsRead16(GUEST_TR_SELECTOR);
            break;
        }
        case INSTRUCTION_INFORMATION_IDENTITY_LLDT:
        {
            VmxVmcsWrite16(GUEST_LDTR_SELECTOR, *selector);
            break;
        }
        case INSTRUCTION_INFORMATION_IDENTITY_LTR:
        {
            VmxVmcsWrite16(GUEST_TR_SELECTOR, *selector);
            break;
        }
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


VOID
InterruptEmulate(
    _In_ UINT32   interruptInformation,  //VmxVmcsRead32(EXIT_INTERRUPTION_INFORMATION)
    _In_ UINT32   errorCode,             //VmxVmcsRead32(EXIT_INTERRUPTION_ERRORCODE)
    _In_ UINT_PTR exitQualification
)
{
    INTERRUPT_INFORMATION interruptInfo;
    
    interruptInfo.u.raw = interruptInformation;

    VmxVmcsWrite32(VM_ENTRY_INTERRUPTION_INFORMATION, interruptInformation);

    if (interruptInfo.u.f.vector == 14)
    {
        __writecr2(exitQualification);
    }

    if (interruptInfo.u.f.hasError)
    {
        VmxVmcsWrite32(VM_ENTRY_EXCEPTION_ERRORCODE, errorCode);
    }

    if ((interruptInfo.u.f.vector == 1) || (interruptInfo.u.f.vector == 3) || (interruptInfo.u.f.vector == 4)) //Technically, vector 1 can be either a trap or a fault... should check dr6[BS] flag
    {
        VmxVmcsWrite32(VM_ENTRY_INSTRUCTION_LENGTH, VmxVmcsRead32(EXIT_INSTRUCTION_LENGTH));
    }
}

VOID
InstrRipAdvance(
    _In_ PREGISTERS regs
)
{
    UINT32 instrLength;

    instrLength = VmxVmcsRead32(EXIT_INSTRUCTION_LENGTH);
    regs->rip += instrLength;
}
