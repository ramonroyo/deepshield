#include "wdk7.h"
#include "exits.h"
#include "tsc.h"
#include "context.h"
#include "instr.h"
#include "vmx.h"
#include "mmu.h"
#include "smp.h"
#include "mem.h"

typedef enum _IA32_CONTROL_REGISTERS {
    IA32_CTRL_CR0 = 0,
    IA32_CTRL_CR2,
    IA32_CTRL_CR3,
    IA32_CTRL_CR4,
    IA32_CTRL_CR8,
    IA32_CTRL_COUNT
} IA32_CONTROL_REGISTERS;

#define UNSUPPORTED_CR IA32_CTRL_COUNT

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

    Qualification.AsUintN = VmxReadPlatform( EXIT_QUALIFICATION );
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
    UINT_PTR function;
    UINT_PTR subleaf;

    function = Registers->rax;
    subleaf  = Registers->rcx;

    InstrCpuidEmulate( Registers );

    //
    // Disable RTM if available
    //
    if (((function & 0xF) == 0x7) && ((subleaf & 0xFFFFFFFF) == 0))
    {
        Registers->rbx &= ~(1 << 11);
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
    _In_ PGP_REGISTERS Regs,
    _In_ VMX_EXIT_INTERRUPT_INFO InterruptInfo
    )
{
    PHYSICAL_ADDRESS PhysicalAddress = { 0 };
    UINT_PTR Process = 0;
    PUINT8 MappedVa = NULL;
    UINT32 InsLenght;
    BOOLEAN IsRdtsc  = FALSE;
    BOOLEAN IsRdtscp = FALSE;

    //
    // Only interested in exceptions from user-mode.
    //
    if (!MmuIsUserModeAddress((PVOID)Regs->rip)) {

        UINT32 Dpl;
        //
        //  Get current privilege level for the descriptor.
        //
        Dpl = VmxRead32( GUEST_CS_ACCESS_RIGHTS );
        Dpl = (Dpl >> 5) & 3;

        NT_ASSERT( Dpl == 0 );
        goto Inject;
    }

    InsLenght = VmxRead32( EXIT_INSTRUCTION_LENGTH );
    if (InsLenght != 2 && InsLenght != 3 ) {
        goto Inject;
    }

    //
    //  Map (avoidable if KVAS is not enabled and hypervisor follows CR3)
    //
    //  Check: the process CR3 for kernel has the whole address space mapped,
    //  so it might be convenient to change the host CR3 to opportunistically
    //  exit with the most frequently used CR3 to call RDTSC/P.
    //
    Process = VmxReadPlatform( GUEST_CR3 );

    PhysicalAddress = MmuGetPhysicalAddress( Process, (PVOID)Regs->rip );
    if (!PhysicalAddress.QuadPart) {
        goto Inject;
    }

    MappedVa = (PUINT8)MmuMap( PhysicalAddress );
    if (!MappedVa) {
        goto Inject;
    }

    //
    //  Check if offending instruction is RDTSC / RDTSCP.
    //
    if (InsLenght == 2) {
        IsRdtsc = (MappedVa[BYTE_OFFSET(Regs->rip)] == 0x0F
                && MappedVa[BYTE_OFFSET(Regs->rip) + 1] == 0x31);

        if (IsRdtsc) {
            RdtscEmulate( Local, Regs, Process, MappedVa );
            goto Unmap;
        }

    } else {
        IsRdtscp = (MappedVa[BYTE_OFFSET(Regs->rip)] == 0x0F
                 && MappedVa[BYTE_OFFSET(Regs->rip) + 1] == 0x01
                 && MappedVa[BYTE_OFFSET(Regs->rip) + 2] == 0xF9);

        if (IsRdtscp) {
            RdtscpEmulate(Local, Regs, Process, MappedVa );
            goto Unmap;
        }
    }

Inject:
    InjectHardwareException( InterruptInfo );

Unmap:
    if (MappedVa) {
        MmuUnmap( MappedVa );
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
            InterruptInfo.AsUint32 = VmxRead32( EXIT_INTERRUPTION_INFORMATION );

            if (InterruptInfo.Bits.InterruptType == INTERRUPT_HARDWARE_EXCEPTION
                && (VECTOR_INVALID_OPCODE_EXCEPTION == InterruptInfo.Bits.Vector
                    || VECTOR_GENERAL_PROTECTION_EXCEPTION == InterruptInfo.Bits.Vector )) {

                HardwareExceptionHandler( Vcpu->LocalContext, 
                                          Registers,
                                          InterruptInfo );
            }
            else {
                InjectInterruptOrException( InterruptInfo );
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
