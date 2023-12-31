#include "wdk7.h"
#include "hvm.h"
#include "vmx.h"
#include "x86.h"
#include "mmu.h"
#include "vmcsinit.h"
#include "instr.h"

PUINTN
LookupGpr(
    _In_ PGP_REGISTERS Registers,
    _In_ UINT32 gpr
)
{
#ifndef _WIN64
    gpr = 7 - gpr;
#endif

    //
    //  TODO: use new GP_REGISTERS for 32-bits build.
    //
    return (PUINTN)Registers + gpr;
}

VOID
InstrInvdEmulate(
    _In_ PGP_REGISTERS Registers
    )
{
    UNREFERENCED_PARAMETER(Registers);
    __wbinvd();
}

#ifdef _WIN64
BOOLEAN
InstrXsetbvEmulate(
    _In_ PGP_REGISTERS Registers
    )
{
    UINT64 Xcr0Value;

    //
    //  The processor raises a GP# fault on its own if xsetbv is executed for
    //  CPL != 0, so we do not have to emulate the fault here.
    //

    if (Registers->Rcx != 0) {

        //
        //  ECX must be zero since only XCR0 is supported.
        //

        VmInjectGpException( 0 );
        return FALSE;
    }

    if (FALSE == IsXStateEnabled()) {

        //
        //  Only handle XCR0 if the guest have XSAVE enabled.
        //

        VmInjectUdException();
        return FALSE;
    }

    //
    //  TODO: Issue CPUID.Rax == 0xD and read XCR0 to check reserved bits.
    //
    //  ((~((UINT32)Cpuid.Rax)) & Xcr0MaskLow) != (UINT32) (~Cpuid.Rax & Registers->Rax))
    //  ((~((UINT32)Cpuid.Rdx)) & Xcr0MaskHigh) != (UINT32) (~Cpuid.Rdx & Registers->Rdx))
    //

    Xcr0Value = Registers->Rdx << 32 | (Registers->Rax & 0xFFFFFFFF);

    if (((Xcr0Value & XFEATURE_ENABLED_X87) != 0)
          || (Xcr0Value & XFEATURE_ENABLED_AVX &&
             (Xcr0Value & XFEATURE_AVX)
                       != XFEATURE_AVX)
          || (Xcr0Value & XFEATURE_AVX512 &&
             (Xcr0Value & (XFEATURE_AVX512 | XFEATURE_AVX))
                       != (XFEATURE_AVX512 | XFEATURE_AVX))
          || ((Xcr0Value & XFEATURE_ENABLED_BNDREGS) != (Xcr0Value & XFEATURE_ENABLED_BNDCSR))) {

        //
        //  1 .XFEATURE_ENABLED_X87 cannot be cleared.
        //  2. AVX requires SSE.
        //  3. AVX512 requires base AVX, OPMASK, ZMM_HI256 and HI16_ZMM.
        //  5. Intel MPX requires setting both bound register state flags.
        //

        VmInjectGpException( 0 );
        return FALSE;
    }

#ifdef DISABLE_OSXSAVE_TRACKING
    __writecr4( __readcr4() | (VmReadN( GUEST_CR4 ) & CR4_OSXSAVE) );
#endif

    __xsetbv( (UINT32) Registers->Rcx, Xcr0Value );
    return TRUE;
}
#else
BOOLEAN
InstrXsetbvEmulate(
    _In_ PGP_REGISTERS Registers
    )
{
    UNREFERENCED_PARAMETER( Registers );
    return TRUE;
}
#endif

VOID
InstrInvVpidEmulate(
    _In_ PGP_REGISTERS Registers
    )
{
    VPID_CTX ctx = { 0 };

    UNREFERENCED_PARAMETER(Registers);
    AsmVmxInvVpid( INV_ALL_CONTEXTS, &ctx );
}

VOID
InstrCpuidEmulate(
    _In_ PGP_REGISTERS Registers
    )
{
    INT32 CpuRegs[4] = { 0 };

    __cpuidex( (INT32*)CpuRegs, (INT32)Registers->Rax, (INT32)Registers->Rcx );

    Registers->Rax = (UINTN)CpuRegs[0];
    Registers->Rbx = (UINTN)CpuRegs[1];
    Registers->Rcx = (UINTN)CpuRegs[2];
    Registers->Rdx = (UINTN)CpuRegs[3];
}

VOID
InstrMsrReadEmulate(
    _In_ PGP_REGISTERS Registers
    )
{
    UINT64 Value;

    Value = __readmsr( LOW32( Registers->Rcx ) );

    Registers->Rax = (UINTN) LOW32( Value );
    Registers->Rdx = (UINTN) HIGH32( Value );
}

VOID
InstrMsrWriteEmulate(
    _In_ PGP_REGISTERS Registers
    )
{
    UINT64 Value;

    Value = (((UINT64)LOW32( Registers->Rdx )) << 32) | LOW32( Registers->Rax );

    __writemsr( LOW32( Registers->Rcx ), Value );
}

VOID
InstrInvlpgEmulate(
    _In_ UINTN exitQualification
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
    PUINTN Gp = LookupGpr( Registers, Qualification.CrAccess.MoveGp );
    INT32 AccessType = Qualification.CrAccess.AccessType;
    UINTN OperandTsd;
    UINTN QuantumTsd;

    if (CR_ACCESS_TYPE_MOV_TO_CR == AccessType) {

        Cr4.AsUintN = *Gp;
        OperandTsd = Cr4.AsUintN & CR4_TSD;

        //
        //  Save CR4 changes although it is not being used.
        //
        Vcpu->HostState.Cr4 = Cr4;

        //
        //  Remove host owned and enforce fixed 0 and fixed 1 bits, and force
        //  TSD bit as it might be not visible (shadow bit 0).
        //
        Cr4.AsUintN = VmMakeCompliantCr4( VmGetGuestVisibleCr4( Cr4.AsUintN ));
        QuantumTsd = VmReadN( GUEST_CR4 ) & CR4_TSD;

        VmWriteN( GUEST_CR4, Cr4.AsUintN | QuantumTsd );

#ifndef DISABLE_OSXSAVE_TRACKING

        if (Cr4.Bits.OsXsave) {

            if (IsXStateSupported() ) {
               VmWriteN( HOST_CR4, __readcr4() | CR4_OSXSAVE );
            }
        }
        else {
            VmWriteN( HOST_CR4, __readcr4() & ~CR4_OSXSAVE );
        }
#endif

        //
        //  By last update TSD bit as guest visible.
        //
        VmWriteN( CR4_READ_SHADOW,
                  (VmReadN( CR4_READ_SHADOW ) & ~CR4_TSD) | OperandTsd );
    }
    else if (CR_ACCESS_TYPE_MOV_FROM_CR == AccessType ) {
        *Gp = VmReadN( GUEST_CR4 );
    }
}

VOID
InstrCr3Emulate(
    _In_ EXIT_QUALIFICATION_CR Qualification,
    _Inout_ PGP_REGISTERS Registers
    )
{
    PUINTN Gp;
    UINT32 AccessType = Qualification.CrAccess.AccessType;
   
    Gp = LookupGpr( Registers, Qualification.CrAccess.MoveGp );

    if (CR_ACCESS_TYPE_MOV_TO_CR == AccessType) {
        VmWriteN( GUEST_CR3, *Gp );
    }
    else if (CR_ACCESS_TYPE_MOV_FROM_CR == AccessType) {
        *Gp = VmReadN( GUEST_CR3 );
    }
}

/*
VOID
InstrDrEmulate(
    _In_ UINTN   exitQualification,
    _In_ PGP_REGISTERS Registers
)
{
    PUINTN             gpr;
    EXIT_QUALIFICATION_DR data;

    data.u.raw = exitQualification;

    gpr = LookupGpr(Registers, (UINT8)data.u.f.gpr);

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
    _In_ UINTN ExitQualification,
    _In_ PGP_REGISTERS Registers
)
{
    EXIT_QUALIFICATION_IO Io;
    Io.AsUintN = ExitQualification;

    if (Io.Bits.Direction)
    {
        if (Io.Bits.StringInstruction)
        {
            switch (Io.Bits.Size)
            {
            case 1: 
                __inbytestring( (USHORT)Io.Bits.PortNum, 
                                (PUCHAR)Registers->Rdi, 
                                (ULONG)(Io.Bits.Rep ? Registers->Rcx : 1)); 
                break;

            case 2: 
                __inwordstring( (USHORT)Io.Bits.PortNum,
                                (PUSHORT)Registers->Rdi,
                                (ULONG)(Io.Bits.Rep ? Registers->Rcx : 1));
                break;

            case 4:
                __indwordstring( (USHORT)Io.Bits.PortNum,
                                 (PULONG)Registers->Rdi,
                                 (ULONG)(Io.Bits.Rep ? Registers->Rcx : 1));
                break;
            }
        }
        else {
            switch (Io.Bits.Size)
            {
#ifndef _WIN64
            case 1: 
                Registers->Rax = (Registers->Rax & 0xFFFFFF00) | __inbyte (Io.Bits.PortNum);
                break;

            case 2:
                Registers->Rax = (Registers->Rax & 0xFFFF0000) | __inword (Io.Bits.PortNum);
                break;
            
            case 4:
                Registers->Rax = __indword(Io.Bits.PortNum);
                break;
#else
            case 1: 
                Registers->Rax = (Registers->Rax & 0xFFFFFFFFFFFFFF00) 
                                 | __inbyte ((USHORT)Io.Bits.PortNum);
                break;

            case 2:
                Registers->Rax = (Registers->Rax & 0xFFFFFFFFFFFF0000)
                                 | __inword ((USHORT)Io.Bits.PortNum);
                break;

            case 4: 
                Registers->Rax = (Registers->Rax & 0xFFFFFFFF00000000)
                                 | __indword((USHORT)Io.Bits.PortNum);
                break;
#endif
            }
        }

    } else {

        if (Io.Bits.StringInstruction) {
            switch (Io.Bits.Size)
            {
            case 1: 
                __outbytestring( (USHORT)Io.Bits.PortNum, 
                                 (PUCHAR)Registers->Rsi,
                                 (ULONG)(Io.Bits.Rep ? Registers->Rcx : 1));
                break;

            case 2: 
                __outwordstring( (USHORT)Io.Bits.PortNum,
                                 (PUSHORT)Registers->Rsi,
                                 (ULONG)(Io.Bits.Rep ? Registers->Rcx : 1));
                break;

            case 4: 
                __outdwordstring( (USHORT)Io.Bits.PortNum,
                                  (PULONG)Registers->Rsi,
                                  (ULONG)(Io.Bits.Rep ? Registers->Rcx : 1)); 
                break;
            }
        }
        else
        {
            switch (Io.Bits.Size)
            {
            case 1: 
                __outbyte( (USHORT)Io.Bits.PortNum, (UCHAR) Registers->Rax );
                break;

            case 2: 
                __outword( (USHORT)Io.Bits.PortNum, (USHORT)Registers->Rax );
                break;

            case 4:
                __outdword( (USHORT)Io.Bits.PortNum, (ULONG)Registers->Rax);
                break;
            }
        }
    }
}

BOOLEAN 
IsSingleStep(
    _In_ PGP_REGISTERS Registers
    )
{
    return Registers->Rflags.Bits.tf == TRUE;
}

VOID 
InjectInt1(
    _In_ PGP_REGISTERS Registers
    )
{
    VMX_EXIT_INTERRUPT_INFO Int1 = { 0 };

    Int1.Bits.InterruptType = INTERRUPT_TYPE_HARDWARE_EXCEPTION;
    Int1.Bits.Valid = 1;
    Int1.Bits.Vector = VECTOR_DEBUG_EXCEPTION;
    
    //
    //  Clear trap flag.
    //
    Registers->Rflags.Bits.tf = 0;

    VmWrite32(VM_ENTRY_INTERRUPTION_INFORMATION, Int1.AsUint32);
}

VOID
InstrRipAdvance(
    _In_ PGP_REGISTERS Registers
    )
{
    UINT32 InstructionLength;

    InstructionLength = VmRead32(EXIT_INSTRUCTION_LENGTH);

    Registers->Rip += InstructionLength;

    //
    //  Single step emulation (has TF enabled).
    //
    if (IsSingleStep(Registers))
    {
        //
        //  Injects an int1 and clears trap flag.
        //
        InjectInt1(Registers);
    }
}
