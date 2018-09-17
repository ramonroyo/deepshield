#include "wdk7.h"
#include "vmcs.h"
#include "vmx.h"
#include "mmu.h"
#include "vmcsinit.h"

#define LOW32(v)  ((UINT32)(v))

VOID
VmcsSetGuestFields(
    VOID
    )
{
    UINT16 Selector;

    VmWriteN( GUEST_CR0, __readcr0());
    VmWriteN( GUEST_CR3, __readcr3());
    VmWriteN( GUEST_CR4, __readcr4());
    VmWriteN( GUEST_DR7, __readdr(7));

    Selector = AsmReadCs();
    VmWrite16( GUEST_CS_SELECTOR, Selector);
    VmWriteN( GUEST_CS_BASE, BaseFromSelector(Selector));
    VmWrite32( GUEST_CS_LIMIT, AsmLimitFromSelector(Selector));
    VmWrite32( GUEST_CS_ACCESS_RIGHTS, ArFromSelector(Selector));

    Selector = AsmReadSs();
    VmWrite16( GUEST_SS_SELECTOR, Selector);
    VmWriteN( GUEST_SS_BASE, BaseFromSelector(Selector));
    VmWrite32( GUEST_SS_LIMIT, AsmLimitFromSelector(Selector));
    VmWrite32( GUEST_SS_ACCESS_RIGHTS, ArFromSelector(Selector));

    Selector = AsmReadDs();
    VmWrite16( GUEST_DS_SELECTOR, Selector);
    VmWriteN( GUEST_DS_BASE, BaseFromSelector(Selector));
    VmWrite32( GUEST_DS_LIMIT, AsmLimitFromSelector(Selector));
    VmWrite32( GUEST_DS_ACCESS_RIGHTS, ArFromSelector(Selector));

    Selector = AsmReadEs();
    VmWrite16( GUEST_ES_SELECTOR, Selector);
    VmWriteN( GUEST_ES_BASE, BaseFromSelector(Selector));
    VmWrite32( GUEST_ES_LIMIT, AsmLimitFromSelector(Selector));
    VmWrite32( GUEST_ES_ACCESS_RIGHTS, ArFromSelector(Selector));

    VmWrite16( GUEST_FS_SELECTOR, AsmReadFs());
#ifndef _WIN64
    VmWriteN( GUEST_FS_BASE, BaseFromSelector(AsmReadFs()));
#else
    VmWriteN( GUEST_FS_BASE, __readmsr(IA32_FS_BASE));
#endif
    VmWrite32( GUEST_FS_LIMIT, AsmLimitFromSelector(AsmReadFs()));
    VmWrite32( GUEST_FS_ACCESS_RIGHTS, ArFromSelector(AsmReadFs()));

    VmWrite16( GUEST_GS_SELECTOR, AsmReadGs());
#ifndef _WIN64
    VmWriteN( GUEST_GS_BASE, BaseFromSelector(AsmReadGs()));
#else
    VmWriteN( GUEST_GS_BASE, __readmsr(IA32_GS_BASE) );
#endif
    VmWrite32( GUEST_GS_LIMIT, AsmLimitFromSelector(AsmReadGs()));
    VmWrite32( GUEST_GS_ACCESS_RIGHTS, ArFromSelector(AsmReadGs()));

    VmWrite16( GUEST_LDTR_SELECTOR, AsmReadLdtr());
    VmWriteN( GUEST_LDTR_BASE, BaseFromSelector(AsmReadLdtr()));
    VmWrite32( GUEST_LDTR_LIMIT, AsmLimitFromSelector(AsmReadLdtr()));
    VmWrite32( GUEST_LDTR_ACCESS_RIGHTS, ArFromSelector(AsmReadLdtr()));

    VmWrite16( GUEST_TR_SELECTOR, AsmReadTr());
    VmWriteN( GUEST_TR_BASE, BaseFromSelector(AsmReadTr()));
    VmWrite32( GUEST_TR_LIMIT, AsmLimitFromSelector(AsmReadTr()));
    VmWrite32( GUEST_TR_ACCESS_RIGHTS, ArFromSelector(AsmReadTr()));

    VmWriteN( GUEST_GDTR_BASE, GetGdtrBase());
    VmWrite32( GUEST_GDTR_LIMIT, GetGdtrLimit());
    VmWriteN( GUEST_IDTR_BASE, GetIdtrBase());
    VmWrite32( GUEST_IDTR_LIMIT, GetIdtrLimit());

    VmWrite64( GUEST_IA32_DEBUGCTL, __readmsr(IA32_DEBUGCTL));

    VmWrite32( GUEST_IA32_SYSENTER_CS, (UINT32)  __readmsr(IA32_SYSENTER_CS));
    VmWriteN( GUEST_IA32_SYSENTER_ESP, (UINTN)__readmsr(IA32_SYSENTER_ESP));
    VmWriteN( GUEST_IA32_SYSENTER_EIP, (UINTN)__readmsr(IA32_SYSENTER_EIP));
    VmWrite64( GUEST_IA32_PERF_GLOBAL_CTRL, (UINTN)__readmsr(IA32_PERF_GLOBAL_CTRL) );
    VmWrite64( GUEST_IA32_EFER, (UINTN)__readmsr( IA32_MSR_EFER ) );
    VmWrite64( VMCS_LINK_POINTER, MAXULONG_PTR );

    //VmWrite64(      GUEST_IA32_PAT,                               X);
    //VmWrite64(      GUEST_IA32_BNDCFGS,                           X);
    //VmWrite32(      GUEST_SMBASE,                                 X);
    //VmWrite32(      GUEST_ACTIVITY_STATE,                         0);
    //VmWrite32(      GUEST_INTERRUPTIBILITY_STATE,                 0);
    //VmWriteN(GUEST_PENDING_DEBUG_EXCEPTIONS,               X);
    //VmWrite32(      VMX_PREEMPTION_TIMER_VALUE,                   X);
    //VmWrite64(      GUEST_PDPTE_0,                                X);
    //VmWrite64(      GUEST_PDPTE_1,                                X);
    //VmWrite64(      GUEST_PDPTE_2,                                X);
    //VmWrite64(      GUEST_PDPTE_3,                                X);
    //VmWrite16(      GUEST_INTERRUPT_STATUS,                       X);
    //VmWrite16(      GUEST_PML_INDEX,                              X);
}

VOID
VmcsSetHostField(
    _In_ UINTN SystemCr3
    )
{
    VmWriteN( HOST_CR3, SystemCr3 );
    VmWriteN( HOST_CR0, VmcsMakeCompliantCr0( __readcr0() ));
    VmWriteN( HOST_CR4, VmcsMakeCompliantCr4( __readcr4() ));

    VmWriteN( HOST_GDTR_BASE, GetGdtrBase() );
    VmWriteN( HOST_IDTR_BASE, GetIdtrBase() );

    VmWrite16( HOST_CS_SELECTOR, AsmReadCs() & 0xFFFC);
    VmWrite16( HOST_SS_SELECTOR, AsmReadSs() & 0xFFFC);
    VmWrite16( HOST_DS_SELECTOR, AsmReadDs() & 0xFFFC);
    VmWrite16( HOST_ES_SELECTOR, AsmReadEs() & 0xFFFC);

    VmWrite16( HOST_FS_SELECTOR, AsmReadFs() & 0xFFFC);
#ifndef _WIN64
    VmWriteN( HOST_FS_BASE, BaseFromSelector(AsmReadFs()));
#else
    VmWriteN( HOST_FS_BASE, __readmsr(IA32_FS_BASE));
#endif

    VmWrite16( HOST_GS_SELECTOR, AsmReadGs() & 0xFFFC);
#ifndef _WIN64
    VmWriteN( HOST_GS_BASE, BaseFromSelector(AsmReadGs()));
#else
    VmWriteN( HOST_GS_BASE, __readmsr(IA32_GS_BASE));
#endif

    VmWrite16( HOST_TR_SELECTOR, AsmReadTr() & 0xFFFC);
    VmWriteN( HOST_TR_BASE, BaseFromSelector(AsmReadTr()));

    //
    //  TODO: check if this is possible always.
    //
    VmWrite32( HOST_IA32_SYSENTER_CS, (UINT32)  __readmsr(IA32_SYSENTER_CS));
    VmWriteN( HOST_IA32_SYSENTER_ESP, (UINTN)__readmsr(IA32_SYSENTER_ESP));
    VmWriteN( HOST_IA32_SYSENTER_EIP, (UINTN)__readmsr(IA32_SYSENTER_EIP));
    //VmWrite64(      HOST_IA32_PERF_GLOBAL_CTRL,                   X);
    //VmWrite64(      HOST_IA32_PAT,                                X);
    //VmWrite64(      HOST_IA32_EFER,                               X);
}

#define HMV_VPID    0x01B0

VOID
VmcsSetControlField(
    VOID
    )
{
    VMX_PIN_EXECUTION_CTLS  pinControls;
    VMX_PROC_PRIMARY_CTLS   procPrimaryControls;
    VMX_PROC_SECONDARY_CTLS procSecondaryControls;
    VMX_EXIT_CTLS           exitControls;
    VMX_ENTRY_CTLS          entryControls;

    pinControls.AsUint32 = 0;
    procPrimaryControls.AsUint32 = 0;
    procSecondaryControls.AsUint32 = 0;
    exitControls.AsUint32 = 0;
    entryControls.AsUint32 = 0;

    // TODO: is this mandatory?.
    procPrimaryControls.Bits.SecondaryControl = 1;
    
    procSecondaryControls.Bits.EnableRdtscp = 1;

    if (IsXStateSupported()) {
        //
        // Expose XSAVES only when XSAVE is supported.
        //
        procSecondaryControls.Bits.EnableXsavesXrstors = 1;
    }

    //
    //  TODO: enable conditionally.
    //
    procSecondaryControls.Bits.EnableInvpcid = 1;

    //
    //  No flush of TLBs on VM entry or VM exit if VPID active. Tagged TLB
    //  entries of different virtual machines can all coexist in the TLB.
    //
    //procSecondaryControls.u.f.enableVpid = 1;

#ifdef _WIN64
    exitControls.Bits.Ia32eHost = 1;
    entryControls.Bits.Ia32eGuest = 1;
#endif

    VmWrite32( VM_EXEC_CONTROLS_PIN_BASED, pinControls.AsUint32);
    VmWrite32( VM_EXEC_CONTROLS_PROC_PRIMARY, procPrimaryControls.AsUint32);
    VmWrite32( VM_EXEC_CONTROLS_PROC_SECONDARY, procSecondaryControls.AsUint32 );
    //VmWrite16( VM_VPID, HMV_VPID );
    VmWrite32( VM_EXIT_CONTROLS, exitControls.AsUint32 );
    VmWrite32( VM_ENTRY_CONTROLS, entryControls.AsUint32 );

    VmWrite32( CR3_TARGET_COUNT, 0 );
    VmWriteN( CR3_TARGET_0, 0 );
    VmWriteN( CR3_TARGET_1, 0 );
    VmWriteN( CR3_TARGET_2, 0 );
    VmWriteN( CR3_TARGET_3, 0 );

    VmWrite32( VM_EXIT_MSR_STORE_COUNT, 0 );
    VmWrite32( VM_EXIT_MSR_LOAD_COUNT, 0 );
    VmWrite32( VM_ENTRY_MSR_LOAD_COUNT, 0 );

    //
    //  XSAVES causes a VM exit if any bit is set in the logical-AND of the
    //  following three values: EDX:EAX, the IA32_XSS MSR, and the XSS-exiting
    //  bitmap.
    //
    // TODO: VmWrite64(      XSS_EXITING_BITMAP,                    X);
    //
    //  Activate VMCS shadow:
    //  TODO: VmWrite64(      VMREAD_BITMAP_ADDRESS,                X);
    //  TODO: VmWrite64(      VMWRITE_BITMAP_ADDRESS,               X);


    //VmWrite32(      EXCEPTION_BITMAP,                             X);
    //VmWrite32(      PAGE_FAULT_ERRORCODE_MASK,                    X);
    //VmWrite32(      PAGE_FAULT_ERRORCODE_MATCH,                   X);
    //VmWrite64(      IO_A_BITMAP_ADDRESS,                          X);
    //VmWrite64(      IO_B_BITMAP_ADDRESS,                          X);
    //VmWrite64(      TSC_OFFSET,                                   X);
    //VmWrite64(      TSC_MULTIPLIER,                               X);
    //VmWriteN(CR0_GUEST_HOST_MASK,                          X);
    //VmWriteN(CR0_READ_SHADOW,                              X);
    //VmWriteN(CR4_GUEST_HOST_MASK,                          X);
    //VmWriteN(CR4_READ_SHADOW,                              X);
    //VmWrite64(      APIC_ACCESS_ADDRESS,                          X);
    //VmWrite64(      VIRTUAL_APIC_ADDRESS,                         X);
    //VmWrite32(      TPR_THRESHOLD,                                X);
    //VmWrite64(      EOI_EXIT_BITMAP_0,                            X);
    //VmWrite64(      EOI_EXIT_BITMAP_1,                            X);
    //VmWrite64(      EOI_EXIT_BITMAP_2,                            X);
    //VmWrite64(      EOI_EXIT_BITMAP_3,                            X);
    //VmWrite16(      POSTED_INTERRUPT_NOTIFICATION_VECTOR,         X);
    //VmWrite64(      POSTED_INTERRUPT_DESCRIPTOR_ADDRESS,          X);
    //VmWrite64(      EXECUTIVE_VMCS_POINTER,                       X);
    //VmWrite64(      EPT_POINTER,                                  X);
    //VmWrite32(      PLE_GAP,                                      X);
    //VmWrite32(      PLE_WINDOW,                                   X);
    //VmWrite64(      VM_FUNCTION_CONTROLS,                         X);
    //VmWrite64(      EPTP_LIST_ADDRESS,                            X);
    //VmWrite64(      ENCLS_EXITING_BITMAP_ADDRESS,                 X);
    //VmWrite64(      PML_ADDRESS,                                  X);
    //VmWrite64(      VIRTUALIZATION_EXCEPTION_INFORMATION_ADDRESS, X);
    //VmWrite16(      EPTP_INDEX,                                   X);
    //VmWrite64(      VM_EXIT_MSR_STORE_ADDRESS,                    X);
    //VmWrite64(      VM_EXIT_MSR_LOAD_ADDRESS,                     X);
    //VmWrite32(      VM_ENTRY_MSR_LOAD_COUNT,                      X);
    //VmWrite64(      VM_ENTRY_MSR_LOAD_ADDRESS,                    X);
}

VOID
VmcsConfigureCommonEntry(
    _In_ UINTN Rip,
    _In_ UINTN Rsp,
    _In_ FLAGS_REGISTER Rflags
    )
{
    //
    //  TODO: why?
    //
    Rflags.Bits.cf = 0;
    Rflags.Bits.zf = 0;

    VmWriteN( GUEST_RSP, Rsp );
    VmWriteN( GUEST_RIP, Rip );
    VmWriteN( GUEST_RFLAGS, Rflags.AsUintN );
    VmWriteN( GUEST_CR3, __readcr3() );
}

#define BITMAP_CLR64(__word, __mask) ((__word) &= ~(UINT64)(__mask))
#define BITMAP_GET64(__word, __mask) ((__word) & (UINT64)(__mask))

/*
FORCEINLINE
VOID
EnableFxOps(
    VOID
    )
{
    UINTN cr0_value = __read_cr0();

    //
    //  Clear TS bit, since we need to operate on XMM registers.
    //
    BITMAP_CLR64( cr0_value, CR0_TS );
    BITMAP_CLR64( cr0_value, CR0_MP );

    __write_cr0( cr0_value );
}*/

/*
void hmm_set_required_values_to_control_registers(void)
{
    hw_write_cr0(hw_read_cr0() | HMM_WP_BIT_MASK);
    efer_msr_set_nxe();     /* Make sure EFER.NXE is set */
/*}*/

/*MON_ASSERT( vmcs_hw_is_cpu_vmx_capable() );

/* init CR0/CR4 to the VMX compatible values */
/*hw_write_cr0( vmcs_hw_make_compliant_cr0( hw_read_cr0() ) );
if ( g_is_post_launch ) {
    /* clear TS bit, since we need to operate on XMM registers. */
/*    enable_fx_ops();
    }
hw_write_cr4( vmcs_hw_make_compliant_cr4( hw_read_cr4() ) );*/