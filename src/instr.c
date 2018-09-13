#include "wdk7.h"
#include "hvm.h"
#include "vmx.h"
#include "x86.h"
#include "mmu.h"

#define LOW32(X)  ((UINT32) ((UINT64)(X))       )
#define HIGH32(X) ((UINT32)(((UINT64)(X)) >> 32))

PUINT_PTR
LookupGp(
    _In_ PGP_REGISTERS Registers,
    _In_ UINT32     gpr
)
{
#ifndef _WIN64
    gpr = 7 - gpr;
#endif

    //
    //  TODO: use new GP_REGISTERS for 32-bits build.
    //
    return (PUINT_PTR)Registers + gpr;
}

VOID
InstrInvdEmulate(
    _In_ PGP_REGISTERS Registers
    )
{
    UNREFERENCED_PARAMETER(Registers);
    __wbinvd();
}

/*
BOOLEAN
IsXStateSupported(
    VOID
    )
{
    UINT32  Eax;
    UINT32  Ebx;
    UINT32  Ecx;
    UINT32  Edx;

    AsmCpuid( CPUID_FEATURE_INFORMATION,
              &Eax,
              &Ebx,
              &Ecx,
              &Edx
    );

    if ((Ecx & BIT26) == 0) {
        return FALSE;
    }
    else {
        return TRUE;
    }
}

//
//  This function return if processor enable XState.
//
BOOLEAN
IsXStateEnabled(
    VOID
    )
{
    if ((AsmReadCr4 () & CR4_OSXSAVE) != 0) {
        return TRUE;

    } else {
        return FALSE;
    }
}*/

VOID
InstrXsetbvEmulate(
    _In_ PGP_REGISTERS Registers
    )
{
#ifdef _WIN64

    /*
    if (IsXStateSupoprted()) {
        AsmWriteCr4(AsmReadCr4() | CR4_OSXSAVE);
    }*/

    __xsetbv((UINT32)Registers->rcx, Registers->rdx << 32 | (UINT32)Registers->rax);

#else
    UNREFERENCED_PARAMETER(Registers);
#endif
}

VOID
InstrInvVpidEmulate(
    _In_ PGP_REGISTERS Registers
    )
{
    VPID_CTX ctx = { 0 };

    UNREFERENCED_PARAMETER(Registers);
    VmxInvVpid( INV_ALL_CONTEXTS, &ctx );
}

VOID
InstrCpuidEmulate(
    _In_ PGP_REGISTERS Registers
)
{
    INT32 CpuRegs[4] = { 0 };

    __cpuidex( (INT32*)CpuRegs,
               (INT32)Registers->rax & 0xFFFFFFFF,
               (INT32)Registers->rcx & 0xFFFFFFFF );

    Registers->rax = (UINT_PTR)CpuRegs[0];
    Registers->rbx = (UINT_PTR)CpuRegs[1];
    Registers->rcx = (UINT_PTR)CpuRegs[2];
    Registers->rdx = (UINT_PTR)CpuRegs[3];
}

VOID
InstrMsrReadEmulate(
    _In_ PGP_REGISTERS Registers
)
{
    UINT64 value;

    value = __readmsr(LOW32(Registers->rcx));

    Registers->rax = (UINT_PTR) LOW32(value);
    Registers->rdx = (UINT_PTR) HIGH32(value);
}

VOID
InstrMsrWriteEmulate(
    _In_ PGP_REGISTERS Registers
)
{
    UINT64 value;

    value = (((UINT64)LOW32(Registers->rdx)) << 32) | LOW32(Registers->rax);

    __writemsr( LOW32(Registers->rcx), value );
}

VOID
InstrInvlpgEmulate(
    _In_ UINT_PTR exitQualification
)
{
    __invlpg((PVOID)exitQualification);
}

VOID
InstrCr4Emulate(
    _In_ EXIT_QUALIFICATION_CR Qualification,
    _Inout_ PGP_REGISTERS Registers,
    _Inout_ PHVM_VCPU Vcpu
    )
{
    CR4_REGISTER Cr4;
    PUINT_PTR Gp = LookupGp( Registers, Qualification.CrAccess.MoveGp );
    INT32 AccessType = Qualification.CrAccess.AccessType;

    if (CR_ACCESS_TYPE_MOV_TO_CR == AccessType) {

        Cr4.AsUintN = *Gp;

        //
        //  Save CR4 changes into the Vcpu although it is not used.
        //
        Vcpu->SavedState.cr4 = Cr4;

        //
        //  Remove host owned and force fixed 0 and fixed 1 bits.
        // 
        Cr4.AsUintN &= ~VmxReadPlatform( CR4_GUEST_HOST_MASK );
        Cr4.AsUintN |= (UINT_PTR)(__readmsr(IA32_VMX_CR4_FIXED0) 
                                & __readmsr(IA32_VMX_CR4_FIXED1));

        VmxWritePlatform( GUEST_CR4, Cr4.AsUintN );
    }
    else if (CR_ACCESS_TYPE_MOV_FROM_CR == AccessType ) {
        *Gp = VmxReadPlatform( GUEST_CR4 );
    }
}

VOID
InstrCr3Emulate(
    _In_ EXIT_QUALIFICATION_CR Qualification,
    _Inout_ PGP_REGISTERS Registers
    )
{
    PUINT_PTR Gp;
    UINT32 AccessType = Qualification.CrAccess.AccessType;
   
    Gp = LookupGp( Registers, Qualification.CrAccess.MoveGp );

    if (CR_ACCESS_TYPE_MOV_TO_CR == AccessType) {
        VmxWritePlatform( GUEST_CR3, *Gp );
    }
    else if (CR_ACCESS_TYPE_MOV_FROM_CR == AccessType) {
        *Gp = VmxReadPlatform( GUEST_CR3 );
    }
}

/*
VOID
InstrDrEmulate(
    _In_ UINT_PTR   exitQualification,
    _In_ PGP_REGISTERS Registers
)
{
    PUINT_PTR             gpr;
    EXIT_QUALIFICATION_DR data;

    data.u.raw = exitQualification;

    gpr = LookupGp(Registers, (UINT8)data.u.f.gpr);

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
    _In_ PGP_REGISTERS Registers
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
            case 1: __inbytestring ((USHORT)data.u.f.port, (PUCHAR) Registers->rdi, (ULONG)(data.u.f.hasRepPrefix ? Registers->rcx : 1)); break;
            case 2: __inwordstring ((USHORT)data.u.f.port, (PUSHORT)Registers->rdi, (ULONG)(data.u.f.hasRepPrefix ? Registers->rcx : 1)); break;
            case 4: __indwordstring((USHORT)data.u.f.port, (PULONG)Registers->rdi,  (ULONG)(data.u.f.hasRepPrefix ? Registers->rcx : 1)); break;
            }
        }
        else
        {
            switch (data.u.f.size)
            {
#ifndef _WIN64
            case 1: Registers->rax = (Registers->rax & 0xFFFFFF00) | __inbyte (data.u.f.port);
            case 2: Registers->rax = (Registers->rax & 0xFFFF0000) | __inword (data.u.f.port);
            case 4: Registers->rax =                            __indword(data.u.f.port);
#else
            case 1: Registers->rax = (Registers->rax & 0xFFFFFFFFFFFFFF00) | __inbyte ((USHORT)data.u.f.port);
            case 2: Registers->rax = (Registers->rax & 0xFFFFFFFFFFFF0000) | __inword ((USHORT)data.u.f.port);
            case 4: Registers->rax = (Registers->rax & 0xFFFFFFFF00000000) | __indword((USHORT)data.u.f.port);
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
            case 1: __outbytestring ((USHORT)data.u.f.port, (PUCHAR) Registers->rsi, (ULONG)(data.u.f.hasRepPrefix ? Registers->rcx : 1)); break;
            case 2: __outwordstring ((USHORT)data.u.f.port, (PUSHORT)Registers->rsi, (ULONG)(data.u.f.hasRepPrefix ? Registers->rcx : 1)); break;
            case 4: __outdwordstring((USHORT)data.u.f.port, (PULONG) Registers->rsi, (ULONG)(data.u.f.hasRepPrefix ? Registers->rcx : 1)); break;
            }
        }
        else
        {
            switch (data.u.f.size)
            {
            case 1: __outbyte ((USHORT)data.u.f.port, (UCHAR) Registers->rax);  break;
            case 2: __outword ((USHORT)data.u.f.port, (USHORT)Registers->rax); break;
            case 4: __outdword((USHORT)data.u.f.port, (ULONG) Registers->rax); break;
            }
        }
    }
}

BOOLEAN 
IsSingleStep(
    _In_ PGP_REGISTERS Regs
    )
{
    return Regs->rflags.u.f.tf == TRUE;
}

VOID 
InjectInt1(
    _In_ PGP_REGISTERS Regs
    )
{
    VMX_EXIT_INTERRUPT_INFO Int1 = { 0 };

    Int1.Bits.InterruptType = INTERRUPT_TYPE_HARDWARE_EXCEPTION;
    Int1.Bits.Valid = 1;
    Int1.Bits.Vector = VECTOR_DEBUG_EXCEPTION;
    
    //
    //  Clear trap flag.
    //
    Regs->rflags.u.f.tf = 0;

    VmxWrite32(VM_ENTRY_INTERRUPTION_INFORMATION, Int1.AsUint32);
}

VOID
InstrRipAdvance(
    _In_ PGP_REGISTERS Regs
    )
{
    UINT32 InstructionLength;

    InstructionLength = VmxRead32(EXIT_INSTRUCTION_LENGTH);

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
