#include "wdk7.h"
#include "vmcs.h"
#include "vmx.h"
#include "mmu.h"

#define LOW32(v)  ((UINT32)(v))

VOID
VmcSetGuestFields(
    VOID
    )
{
    UINT16 Selector;

    VmxWritePlatform( GUEST_CR0, __readcr0());
    VmxWritePlatform( GUEST_CR3, __readcr3());
    VmxWritePlatform( GUEST_CR4, __readcr4());
    VmxWritePlatform( GUEST_DR7, __readdr(7));

    Selector = AsmReadCs();
    VmxWrite16( GUEST_CS_SELECTOR, Selector);
    VmxWritePlatform( GUEST_CS_BASE, BaseFromSelector(Selector));
    VmxWrite32( GUEST_CS_LIMIT, AsmLimitFromSelector(Selector));
    VmxWrite32( GUEST_CS_ACCESS_RIGHTS, ArFromSelector(Selector));

    Selector = AsmReadSs();
    VmxWrite16( GUEST_SS_SELECTOR, Selector);
    VmxWritePlatform( GUEST_SS_BASE, BaseFromSelector(Selector));
    VmxWrite32( GUEST_SS_LIMIT, AsmLimitFromSelector(Selector));
    VmxWrite32( GUEST_SS_ACCESS_RIGHTS, ArFromSelector(Selector));

    Selector = AsmReadDs();
    VmxWrite16( GUEST_DS_SELECTOR, Selector);
    VmxWritePlatform( GUEST_DS_BASE, BaseFromSelector(Selector));
    VmxWrite32( GUEST_DS_LIMIT, AsmLimitFromSelector(Selector));
    VmxWrite32( GUEST_DS_ACCESS_RIGHTS, ArFromSelector(Selector));

    Selector = AsmReadEs();
    VmxWrite16( GUEST_ES_SELECTOR, Selector);
    VmxWritePlatform( GUEST_ES_BASE, BaseFromSelector(Selector));
    VmxWrite32( GUEST_ES_LIMIT, AsmLimitFromSelector(Selector));
    VmxWrite32( GUEST_ES_ACCESS_RIGHTS, ArFromSelector(Selector));

    VmxWrite16( GUEST_FS_SELECTOR, AsmReadFs());
#ifndef _WIN64
    VmxWritePlatform( GUEST_FS_BASE, BaseFromSelector(AsmReadFs()));
#else
    VmxWritePlatform( GUEST_FS_BASE, __readmsr(IA32_FS_BASE));
#endif
    VmxWrite32( GUEST_FS_LIMIT, AsmLimitFromSelector(AsmReadFs()));
    VmxWrite32( GUEST_FS_ACCESS_RIGHTS, ArFromSelector(AsmReadFs()));

    VmxWrite16( GUEST_GS_SELECTOR, AsmReadGs());
#ifndef _WIN64
    VmxWritePlatform( GUEST_GS_BASE, BaseFromSelector(AsmReadGs()));
#else
    VmxWritePlatform( GUEST_GS_BASE, __readmsr(IA32_GS_BASE) );
#endif
    VmxWrite32( GUEST_GS_LIMIT, AsmLimitFromSelector(AsmReadGs()));
    VmxWrite32( GUEST_GS_ACCESS_RIGHTS, ArFromSelector(AsmReadGs()));

    VmxWrite16( GUEST_LDTR_SELECTOR, AsmReadLdtr());
    VmxWritePlatform( GUEST_LDTR_BASE, BaseFromSelector(AsmReadLdtr()));
    VmxWrite32( GUEST_LDTR_LIMIT, AsmLimitFromSelector(AsmReadLdtr()));
    VmxWrite32( GUEST_LDTR_ACCESS_RIGHTS, ArFromSelector(AsmReadLdtr()));

    VmxWrite16( GUEST_TR_SELECTOR, AsmReadTr());
    VmxWritePlatform( GUEST_TR_BASE, BaseFromSelector(AsmReadTr()));
    VmxWrite32( GUEST_TR_LIMIT, AsmLimitFromSelector(AsmReadTr()));
    VmxWrite32( GUEST_TR_ACCESS_RIGHTS, ArFromSelector(AsmReadTr()));

    VmxWritePlatform( GUEST_GDTR_BASE, GetGdtrBase());
    VmxWrite32( GUEST_GDTR_LIMIT, GetGdtrLimit());
    VmxWritePlatform( GUEST_IDTR_BASE, GetIdtrBase());
    VmxWrite32( GUEST_IDTR_LIMIT, GetIdtrLimit());

    VmxWrite64( GUEST_IA32_DEBUGCTL, __readmsr(IA32_DEBUGCTL));

    VmxWrite32( GUEST_IA32_SYSENTER_CS, (UINT32)  __readmsr(IA32_SYSENTER_CS));
    VmxWritePlatform( GUEST_IA32_SYSENTER_ESP, (UINTN)__readmsr(IA32_SYSENTER_ESP));
    VmxWritePlatform( GUEST_IA32_SYSENTER_EIP, (UINTN)__readmsr(IA32_SYSENTER_EIP));
    VmxWrite64( GUEST_IA32_PERF_GLOBAL_CTRL, (UINTN)__readmsr(IA32_PERF_GLOBAL_CTRL) );
    VmxWrite64( GUEST_IA32_EFER, (UINTN)__readmsr( IA32_MSR_EFER ) );
    VmxWrite64( VMCS_LINK_POINTER, MAXULONG_PTR );

    //VmxWrite64(      GUEST_IA32_PAT,                               X);
    //VmxWrite64(      GUEST_IA32_BNDCFGS,                           X);
    //VmxWrite32(      GUEST_SMBASE,                                 X);
    //VmxWrite32(      GUEST_ACTIVITY_STATE,                         0);
    //VmxWrite32(      GUEST_INTERRUPTIBILITY_STATE,                 0);
    //VmxWritePlatform(GUEST_PENDING_DEBUG_EXCEPTIONS,               X);
    //VmxWrite32(      VMX_PREEMPTION_TIMER_VALUE,                   X);
    //VmxWrite64(      GUEST_PDPTE_0,                                X);
    //VmxWrite64(      GUEST_PDPTE_1,                                X);
    //VmxWrite64(      GUEST_PDPTE_2,                                X);
    //VmxWrite64(      GUEST_PDPTE_3,                                X);
    //VmxWrite16(      GUEST_INTERRUPT_STATUS,                       X);
    //VmxWrite16(      GUEST_PML_INDEX,                              X);
}

VOID
VmcSetHostField(
    _In_ UINTN SystemCr3
    )
{
    VmxWritePlatform( HOST_CR3, SystemCr3 );
    VmxWritePlatform( HOST_CR0, ((__readcr0() & LOW32(__readmsr(IA32_VMX_CR0_FIXED1))) 
                                              | LOW32(__readmsr(IA32_VMX_CR0_FIXED0))));
    VmxWritePlatform( HOST_CR4, ((__readcr4() & LOW32(__readmsr(IA32_VMX_CR4_FIXED1)))
                                              | LOW32(__readmsr(IA32_VMX_CR4_FIXED0))));

    VmxWritePlatform( HOST_GDTR_BASE, GetGdtrBase() );
    VmxWritePlatform( HOST_IDTR_BASE, GetIdtrBase() );

    VmxWrite16( HOST_CS_SELECTOR, AsmReadCs() & 0xFFFC);
    VmxWrite16( HOST_SS_SELECTOR, AsmReadSs() & 0xFFFC);
    VmxWrite16( HOST_DS_SELECTOR, AsmReadDs() & 0xFFFC);
    VmxWrite16( HOST_ES_SELECTOR, AsmReadEs() & 0xFFFC);

    VmxWrite16( HOST_FS_SELECTOR, AsmReadFs() & 0xFFFC);
#ifndef _WIN64
    VmxWritePlatform( HOST_FS_BASE, BaseFromSelector(AsmReadFs()));
#else
    VmxWritePlatform( HOST_FS_BASE, __readmsr(IA32_FS_BASE));
#endif

    VmxWrite16( HOST_GS_SELECTOR, AsmReadGs() & 0xFFFC);
#ifndef _WIN64
    VmxWritePlatform( HOST_GS_BASE, BaseFromSelector(AsmReadGs()));
#else
    VmxWritePlatform( HOST_GS_BASE, __readmsr(IA32_GS_BASE));
#endif

    VmxWrite16( HOST_TR_SELECTOR, AsmReadTr() & 0xFFFC);
    VmxWritePlatform( HOST_TR_BASE, BaseFromSelector(AsmReadTr()));

    //
    //  TODO: check if this is possible always.
    //
    VmxWrite32( HOST_IA32_SYSENTER_CS, (UINT32)  __readmsr(IA32_SYSENTER_CS));
    VmxWritePlatform( HOST_IA32_SYSENTER_ESP, (UINTN)__readmsr(IA32_SYSENTER_ESP));
    VmxWritePlatform( HOST_IA32_SYSENTER_EIP, (UINTN)__readmsr(IA32_SYSENTER_EIP));
    //VmxWrite64(      HOST_IA32_PERF_GLOBAL_CTRL,                   X);
    //VmxWrite64(      HOST_IA32_PAT,                                X);
    //VmxWrite64(      HOST_IA32_EFER,                               X);
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

    procPrimaryControls.Bits.SecondaryControl = 1;
    
    procSecondaryControls.Bits.EnableRdtscp = 1;
    procSecondaryControls.Bits.EnableXsavesXrstors = 1;
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

    VmxWrite32( VM_EXEC_CONTROLS_PIN_BASED, pinControls.AsUint32);
    VmxWrite32( VM_EXEC_CONTROLS_PROC_PRIMARY, procPrimaryControls.AsUint32);
    VmxWrite32( VM_EXEC_CONTROLS_PROC_SECONDARY, procSecondaryControls.AsUint32 );
    //VmxWrite16( VM_VPID, HMV_VPID );
    VmxWrite32( VM_EXIT_CONTROLS, exitControls.AsUint32 );
    VmxWrite32( VM_ENTRY_CONTROLS, entryControls.AsUint32 );

    VmxWrite32( CR3_TARGET_COUNT, 0 );
    VmxWritePlatform( CR3_TARGET_0, 0 );
    VmxWritePlatform( CR3_TARGET_1, 0 );
    VmxWritePlatform( CR3_TARGET_2, 0 );
    VmxWritePlatform( CR3_TARGET_3, 0 );

    VmxWrite32( VM_EXIT_MSR_STORE_COUNT, 0 );
    VmxWrite32( VM_EXIT_MSR_LOAD_COUNT, 0 );
    VmxWrite32( VM_ENTRY_MSR_LOAD_COUNT, 0 );

    //
    //  XSAVES causes a VM exit if any bit is set in the logical-AND of the
    //  following three values: EDX:EAX, the IA32_XSS MSR, and the XSS-exiting
    //  bitmap.
    //
    // TODO: VmxWrite64(      XSS_EXITING_BITMAP,                    X);
    //
    //  Activate VMCS shadow:
    //  TODO: VmxWrite64(      VMREAD_BITMAP_ADDRESS,                X);
    //  TODO: VmxWrite64(      VMWRITE_BITMAP_ADDRESS,               X);


    //VmxWrite32(      EXCEPTION_BITMAP,                             X);
    //VmxWrite32(      PAGE_FAULT_ERRORCODE_MASK,                    X);
    //VmxWrite32(      PAGE_FAULT_ERRORCODE_MATCH,                   X);
    //VmxWrite64(      IO_A_BITMAP_ADDRESS,                          X);
    //VmxWrite64(      IO_B_BITMAP_ADDRESS,                          X);
    //VmxWrite64(      TSC_OFFSET,                                   X);
    //VmxWrite64(      TSC_MULTIPLIER,                               X);
    //VmxWritePlatform(CR0_GUEST_HOST_MASK,                          X);
    //VmxWritePlatform(CR0_READ_SHADOW,                              X);
    //VmxWritePlatform(CR4_GUEST_HOST_MASK,                          X);
    //VmxWritePlatform(CR4_READ_SHADOW,                              X);
    //VmxWrite64(      APIC_ACCESS_ADDRESS,                          X);
    //VmxWrite64(      VIRTUAL_APIC_ADDRESS,                         X);
    //VmxWrite32(      TPR_THRESHOLD,                                X);
    //VmxWrite64(      EOI_EXIT_BITMAP_0,                            X);
    //VmxWrite64(      EOI_EXIT_BITMAP_1,                            X);
    //VmxWrite64(      EOI_EXIT_BITMAP_2,                            X);
    //VmxWrite64(      EOI_EXIT_BITMAP_3,                            X);
    //VmxWrite16(      POSTED_INTERRUPT_NOTIFICATION_VECTOR,         X);
    //VmxWrite64(      POSTED_INTERRUPT_DESCRIPTOR_ADDRESS,          X);
    //VmxWrite64(      EXECUTIVE_VMCS_POINTER,                       X);
    //VmxWrite64(      EPT_POINTER,                                  X);
    //VmxWrite32(      PLE_GAP,                                      X);
    //VmxWrite32(      PLE_WINDOW,                                   X);
    //VmxWrite64(      VM_FUNCTION_CONTROLS,                         X);
    //VmxWrite64(      EPTP_LIST_ADDRESS,                            X);
    //VmxWrite64(      ENCLS_EXITING_BITMAP_ADDRESS,                 X);
    //VmxWrite64(      PML_ADDRESS,                                  X);
    //VmxWrite64(      VIRTUALIZATION_EXCEPTION_INFORMATION_ADDRESS, X);
    //VmxWrite16(      EPTP_INDEX,                                   X);
    //VmxWrite64(      VM_EXIT_MSR_STORE_ADDRESS,                    X);
    //VmxWrite64(      VM_EXIT_MSR_LOAD_ADDRESS,                     X);
    //VmxWrite32(      VM_ENTRY_MSR_LOAD_COUNT,                      X);
    //VmxWrite64(      VM_ENTRY_MSR_LOAD_ADDRESS,                    X);
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

    VmxWritePlatform( GUEST_RSP, Rsp );
    VmxWritePlatform( GUEST_RIP, Rip );
    VmxWritePlatform( GUEST_RFLAGS, Rflags.AsUintN );
    VmxWritePlatform( GUEST_CR3, __readcr3() );
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