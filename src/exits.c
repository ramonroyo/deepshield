#include "wdk7.h"
#include "exits.h"
#include "tsc.h"
#include "context.h"
#include "instr.h"
#include "vmx.h"
#include "mmu.h"
#include "smp.h"
#include "mem.h"

/*
VOID
CrAccessEmulate(
    _In_ PREGISTERS regs
)
{
    
    EXIT_QUALIFICATION_CR data;
    PUINT_PTR             gpr;

    //
    // Emulate
    //
    data.u.raw = VmxVmcsReadPlatform(EXIT_QUALIFICATION);

    InstrCrEmulate(data.u.raw, regs);

    //
    // Follow CR3
    //
    gpr = RegsGpr(regs, (UINT8)data.u.f.gpr);

    VmxVmcsWritePlatform(HOST_CR3, *gpr);

    InstrRipAdvance(regs);
}
*/

VOID
Cr4AccessEmulate(
    _In_ PHVM_CORE  core,
    _In_ PREGISTERS regs
)
{
    EXIT_QUALIFICATION_CR data;
    CR4_REGISTER          cr4;

    data.u.raw = VmxVmcsReadPlatform(EXIT_QUALIFICATION);

    //
    // Read new CR4
    //
    cr4.u.raw = *RegsGpr(regs, (UINT8)data.u.f.gpr);

    //
    // Save to host "as is"
    //
    VmxVmcsWritePlatform(HOST_CR4, cr4.u.raw);
    core->savedState.cr4 = cr4;

    //
    // Save to guest enabling and hiding tsd
    //
    cr4.u.f.tsd = 1;
    VmxVmcsWritePlatform(GUEST_CR4, cr4.u.raw);

    //
    // Done
    //
    InstrRipAdvance(regs);
}

VOID
CpuidEmulate(
    _In_ PREGISTERS regs
)
{
    UINT_PTR function;
    UINT_PTR subleaf;

    function = regs->rax;
    subleaf  = regs->rcx;

    InstrCpuidEmulate(regs);

    //
    // Disable RTM if available
    //
    if (((function & 0xF) == 0x7) && ((subleaf & 0xFFFFFFFF) == 0))
    {
        regs->rbx &= ~(1 << 11);
    }

    InstrRipAdvance(regs);
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
}

VOID
PageFaultEmulate(
    _In_ PREGISTERS regs
    )
{
    UINT_PTR exitQualification;

    exitQualification = VmxVmcsReadPlatform(EXIT_QUALIFICATION);

    VmxVmcsWrite32(VM_ENTRY_INTERRUPTION_INFORMATION, VmxVmcsRead32(EXIT_INTERRUPTION_INFORMATION));
    VmxVmcsWrite32(VM_ENTRY_EXCEPTION_ERRORCODE, VmxVmcsRead32(EXIT_INTERRUPTION_ERRORCODE));
    __writecr2(exitQualification);

    if (MmuIsUserModeAddress((PVOID)regs->rip)) {
        // 
    }
}
*/

VOID
GeneralProtectionFaultEmulate(
    _In_ PVOID Local,
    _In_ PREGISTERS Regs
)
{
    PHYSICAL_ADDRESS PhysicalAddress = { 0 };
    UINT_PTR Process = 0;
    PUINT8 Mapping = 0;
    //UINT32 InstructionLength = 0;
    BOOLEAN isRdtsc  = FALSE;
    BOOLEAN isRdtscp = FALSE;

    //
    // Only interested in #GP's from user
    //
    if (!MmuIsUserModeAddress((PVOID)Regs->rip))
        goto inject;

    //
    // Map (could be avoided if KvaShadow is not enabled and hypervisor follows CR3)
    //
    Process = VmxVmcsReadPlatform(GUEST_CR3);

    PhysicalAddress = MmuGetPhysicalAddress(Process, (PVOID)Regs->rip);
    if (!PhysicalAddress.QuadPart)
        goto inject;

    Mapping = (PUINT8)MmuMap(PhysicalAddress);
    if (!Mapping)
        goto inject;

    //
    // Check if offending instruction is RDTSC/RDTSCP
    //
    isRdtsc  = (Mapping[BYTE_OFFSET(Regs->rip)] == 0x0F && Mapping[BYTE_OFFSET(Regs->rip) + 1] == 0x31);
    isRdtscp = (Mapping[BYTE_OFFSET(Regs->rip)] == 0x0F && Mapping[BYTE_OFFSET(Regs->rip) + 1] == 0x01 && Mapping[BYTE_OFFSET(Regs->rip) + 2] == 0xF9);

    //
    // Check to emulate
    //
    if (isRdtsc)
    {
        //
        // Emulate RDTSC
        //
        RdtscEmulate(Local, Regs, Process, Mapping);
        MmuUnmap(Mapping);

        return;

    }
    
    if (isRdtscp)
    {
        //
        // Emulate RDTSCP
        //
        RdtscpEmulate(Local, Regs, Process, Mapping);
        MmuUnmap(Mapping);
        return;

    }

    //
    // Reinject exception
    //
inject:
    VmxVmcsWrite32(VM_ENTRY_INTERRUPTION_INFORMATION, VmxVmcsRead32(EXIT_INTERRUPTION_INFORMATION));
    VmxVmcsWrite32(VM_ENTRY_EXCEPTION_ERRORCODE, VmxVmcsRead32(EXIT_INTERRUPTION_ERRORCODE));
}

VOID 
DsHvdsExitHandler(
    _In_ UINT32     exitReason,
    _In_ PHVM_CORE  core,
    _In_ PREGISTERS regs
    )
{
    //
    // Handle exit reasons
    //
    switch (exitReason)
    {
        //
        // Common VM exits
        //
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
            HvmCoreHandleCommonExits(exitReason, core, regs);
            break;
        }

        //
        // Custom VM exits
        //
        case EXIT_REASON_CPUID:
        {
            CpuidEmulate(regs);
            break;
        }
        case EXIT_REASON_EXCEPTION_OR_NMI:
        {
            GeneralProtectionFaultEmulate(core->localContext, regs);
            break;
        }
        case EXIT_REASON_CR_ACCESS:
        {
            Cr4AccessEmulate(core, regs);
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
            KeBugCheckEx(HYPERVISOR_ERROR, exitReason, 0, 0, 0);
            break;
        }
    }
}
