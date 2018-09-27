#include "wdk7.h"
#include "exits.h"
#include "tsc.h"
#include "context.h"
#include "instr.h"
#include "vmx.h"
#include "mmu.h"
#include "smp.h"
#include "mem.h"

static IA32_CONTROL_REGISTERS LookupCr[] = {
    IA32_CTRL_CR0,
    UNSUPPORTED_CR,
    UNSUPPORTED_CR,
    IA32_CTRL_CR3,
    IA32_CTRL_CR4,
    UNSUPPORTED_CR,
    UNSUPPORTED_CR,
    UNSUPPORTED_CR,
    IA32_CTRL_CR8
};

VOID
CrAccessHandler(
    _In_ PHVM_VCPU Vcpu,
    _In_ PGP_REGISTERS Registers
    )
{
    EXIT_QUALIFICATION_CR Qualification;
    IA32_CONTROL_REGISTERS CrNumber;

    Qualification.AsUintN = VmReadN( EXIT_QUALIFICATION );
    CrNumber = LookupCr[Qualification.CrAccess.Number];

    switch ( CrNumber ) {
        case IA32_CTRL_CR4:

            InstrCr4Emulate( Qualification, Registers, Vcpu );
            break;

        case IA32_CTRL_CR3:

            InstrCr3Emulate( Qualification, Registers );
            break;

        default:

            //
            //  Sanity check to avoid monitoring other operations
            //  accidentally.
            //
            NT_ASSERT( FALSE );
            break;
    }

    InstrRipAdvance( Registers );
}

VOID
CpuidEmulate(
    _In_ PGP_REGISTERS Registers
    )
{
    UINTN function;
    UINTN subleaf;

    function = Registers->Rax;
    subleaf  = Registers->Rcx;

    InstrCpuidEmulate( Registers );

    //
    // Disable RTM if available
    //
    if (((function & 0xF) == 0x7) && ((subleaf & 0xFFFFFFFF) == 0))
    {
        Registers->Rbx &= ~(1 << 11);
    }

    InstrRipAdvance( Registers );
}

/*
VOID
ResetCacheOperatingMode(
    VOID
    )
{
    CR0_REGISTER cr0_original = { 0 };
    CR0_REGISTER cr0_new = { 0 };

    cr0_original.u.raw = __readcr0();
    cr0_new = cr0_original;

    //
    //  Set the no-fill cache mode.
    //
    cr0_new.u.f.cd = 1;
    cr0_new.u.f.nw = 0;

    __writecr0(cr0_new.u.raw);
    __wbinvd();

    //
    //  Restore previous cache mode.
    // 
    __writecr0(cr0_original.u.raw);
}

VOID
ResetSmepMode(
    VOID
    )
{
    CR4_REGISTER cr4_original = { 0 };
    CR4_REGISTER cr4_new = { 0 };

    cr4_original.u.raw = __readcr4();
    cr4_new = cr4_original;

    cr4_new.u.f.smep = 0;
    __writecr4(cr4_new.u.raw);
    __writecr4(cr4_original.u.raw);
}

VOID
FlushCurrentTb(
    VOID
    )
{
    UINT64 cr4;

    cr4 = __readcr4();

    if (cr4 & 0x20080) {
        __writecr4( cr4 ^ 0x80 );
        __writecr4( cr4 );
    }
    else {
        __writecr3( __readcr3() );
    }
}*/

VOID
HardwareExceptionHandler(
    _In_ PVOID Local,
    _In_ PGP_REGISTERS Registers,
    _In_ VMX_EXIT_INTERRUPT_INFO InterruptInfo
    )
{
    PHYSICAL_ADDRESS RipGpa = { 0 };
    UINTN GuestCr3 = 0;
    PUINT8 RipGva = NULL;
    //UINT32 InsLenght = 0;
    BOOLEAN IsRdtsc  = FALSE;
    BOOLEAN IsRdtscp = FALSE;
    UINT32 Dpl = 0;

    //
    //  Skip and reinject exceptions from kernel mode.
    //
    if (!MmuIsUserModeAddress( (PVOID)Registers->Rip) ) {

        //
        //  Get current privilege level for the descriptor.
        //
        Dpl = VmRead32( GUEST_CS_ACCESS_RIGHTS );
        Dpl = (Dpl >> 5) & 3;

        NT_ASSERT( Dpl == 0 );
        goto InjectException;
    }

    //
    //  Fix: sometimes the value read is 0.
    //
    //  InsLenght = VmRead32( EXIT_INSTRUCTION_LENGTH );
    //  if (InsLenght != 2 && InsLenght != 3 ) {
    //      goto Inject;
    //  }
    //

    //
    //  Judas note: avoidable if KVA Shadow is not enabled and hypervisor
    //  follows CR3.
    //
    //  TODO: the process CR3 for kernel has the whole address space mapped,
    //  so it might be convenient to change the host CR3 to opportunistically
    //  exit with the most frequently used CR3 to call RDTSC/P.
    //

    GuestCr3 = VmReadN( GUEST_CR3 );

    RipGpa = MmuGetPhysicalAddress( GuestCr3, (PVOID)Registers->Rip );
    if (!RipGpa.QuadPart) {
        goto InjectException;
    }

    RipGva = (PUINT8)MmuMapPage( RipGpa, FALSE );
    if (!RipGva) {
        goto InjectException;
    }

    //
    //  Check if offending instruction is RDTSC / RDTSCP.
    //

    IsRdtsc = (RipGva[BYTE_OFFSET(Registers->Rip)] == 0x0F
            && RipGva[BYTE_OFFSET(Registers->Rip) + 1] == 0x31);

    if (IsRdtsc) {
        RdtscEmulate( Local, Registers, GuestCr3, RipGva );
        goto UnmapPage;
    }

    IsRdtscp = (RipGva[BYTE_OFFSET(Registers->Rip)] == 0x0F
                && RipGva[BYTE_OFFSET(Registers->Rip) + 1] == 0x01
                && RipGva[BYTE_OFFSET(Registers->Rip) + 2] == 0xF9);

    if (IsRdtscp) {
        RdtscpEmulate( Local, Registers, GuestCr3, RipGva );
        goto UnmapPage;
    }

InjectException:
    VmInjectHardwareException( InterruptInfo );

UnmapPage:

    if (RipGva) {
        MmuUnmapPage( RipGva );
    }
}

//
//  IRQL requirements don't mean much when running in VMM context. For all
//  intents and purposes, even if you enter at PASSIVE_LEVEL, you are 
//  technically at HIGH_LEVEL, as interrupts are disabled. So you should be
//  calling almost zero Windows APIs, except if they are documented to run
//  at HIGH_LEVEL
//
VOID 
DsHvmExitHandler(
    _In_ UINT32 ExitReason,
    _In_ PHVM_VCPU Vcpu,
    _In_ PGP_REGISTERS Registers
    )
{
    //
    //  I thought at first that you had done something clever, but I see that
    //  there was nothing in it, after all.
    //
    switch ( ExitReason )
    {
        case EXIT_REASON_INVD:
        case EXIT_REASON_XSETBV:
        case EXIT_REASON_VMXON:
        case EXIT_REASON_VMXOFF:
        case EXIT_REASON_VMCLEAR:
        case EXIT_REASON_VMRESUME:
        case EXIT_REASON_VMLAUNCH:
        case EXIT_REASON_VMPTRLD:
        case EXIT_REASON_VMPTRST:
        case EXIT_REASON_VMREAD:
        case EXIT_REASON_VMWRITE:
        case EXIT_REASON_INVEPT:
        case EXIT_REASON_INVVPID:
        case EXIT_REASON_MSR_READ:
        case EXIT_REASON_MSR_WRITE:
        {
            HvmVcpuCommonExitsHandler( ExitReason, Vcpu, Registers );
            break;
        }

        case EXIT_REASON_SOFTWARE_INTERRUPT_EXCEPTION_NMI:
        {
            VMX_EXIT_INTERRUPT_INFO InterruptInfo;
            InterruptInfo.AsUint32 = VmRead32( EXIT_INTERRUPTION_INFORMATION );

            if (InterruptInfo.Bits.InterruptType == INTERRUPT_HARDWARE_EXCEPTION
                && (VECTOR_INVALID_OPCODE_EXCEPTION == InterruptInfo.Bits.Vector
                    || VECTOR_GENERAL_PROTECTION_EXCEPTION == InterruptInfo.Bits.Vector )) {

                HardwareExceptionHandler( Vcpu->Context, Registers, InterruptInfo );
            }
            else {
                VmInjectInterruptOrException( InterruptInfo );
            }

            break;
        }

        case EXIT_REASON_CR_ACCESS:
        {
            CrAccessHandler( Vcpu, Registers );
            break;
        }

        case EXIT_REASON_CPUID:
        {
            CpuidEmulate( Registers );
            break;
        }

        case EXIT_REASON_INIT:
        case EXIT_REASON_SIPI:
        case EXIT_REASON_TRIPLE_FAULT:
        {
            break;
        }

        default:
        {
            KeBugCheckEx( HYPERVISOR_ERROR, ExitReason, 0, 0, 0 );
            break;
        }
    }
}
