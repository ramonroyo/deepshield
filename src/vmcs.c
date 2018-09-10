#include "wdk7.h"
#include "vmcs.h"
#include "vmx.h"
#include "mmu.h"

BOOLEAN
VmcsClear(
    _In_ PVOID vmcs
)
{
    VMX_MSR_BASIC        basicVmx;
    PHYSICAL_ADDRESS vmcsPhysical;

    basicVmx.AsUint64 = VmxCapability(IA32_VMX_BASIC);

    *(PUINT32)vmcs = basicVmx.Bits.revisionId;

    vmcsPhysical   = MmuGetPhysicalAddress(0, vmcs);

    return VmxVmClear((unsigned __int64*)&vmcsPhysical) == 0;
}

BOOLEAN
VmcsLoad(
    _In_ PVOID vmcs
)
{
    PHYSICAL_ADDRESS vmcsPhysical;

    vmcsPhysical = MmuGetPhysicalAddress(0, vmcs);

    return VmxVmPtrLd((unsigned __int64*)&vmcsPhysical) == 0;
}

#define LOW32(v)  ((UINT32)(v))

VOID
VmcsConfigureCommonGuest(
    VOID
)
{
    VmxVmcsWritePlatform(GUEST_CR0,                                    __readcr0());
    VmxVmcsWritePlatform(GUEST_CR3,                                    __readcr3());
    VmxVmcsWritePlatform(GUEST_CR4,                                    __readcr4());
    VmxVmcsWritePlatform(GUEST_DR7,                                    __readdr(7));

    VmxVmcsWrite16(      GUEST_CS_SELECTOR,                            __readcs());
    VmxVmcsWritePlatform(GUEST_CS_BASE,                                DescriptorBase(__readcs()));
    VmxVmcsWrite32(      GUEST_CS_LIMIT,                               DescriptorLimit(__readcs()));
    VmxVmcsWrite32(      GUEST_CS_ACCESS_RIGHTS,                       DescriptorAccessRights(__readcs()));

    VmxVmcsWrite16(      GUEST_SS_SELECTOR,                            __readss());
    VmxVmcsWritePlatform(GUEST_SS_BASE,                                DescriptorBase(__readss()));
    VmxVmcsWrite32(      GUEST_SS_LIMIT,                               DescriptorLimit(__readss()));
    VmxVmcsWrite32(      GUEST_SS_ACCESS_RIGHTS,                       DescriptorAccessRights(__readss()));

    VmxVmcsWrite16(      GUEST_DS_SELECTOR,                            __readds());
    VmxVmcsWritePlatform(GUEST_DS_BASE,                                DescriptorBase(__readds()));
    VmxVmcsWrite32(      GUEST_DS_LIMIT,                               DescriptorLimit(__readds()));
    VmxVmcsWrite32(      GUEST_DS_ACCESS_RIGHTS,                       DescriptorAccessRights(__readds()));

    VmxVmcsWrite16(      GUEST_ES_SELECTOR,                            __reades());
    VmxVmcsWritePlatform(GUEST_ES_BASE,                                DescriptorBase(__reades()));
    VmxVmcsWrite32(      GUEST_ES_LIMIT,                               DescriptorLimit(__reades()));
    VmxVmcsWrite32(      GUEST_ES_ACCESS_RIGHTS,                       DescriptorAccessRights(__reades()));

    VmxVmcsWrite16(      GUEST_FS_SELECTOR,                            __readfs());
#ifndef _WIN64
    VmxVmcsWritePlatform(GUEST_FS_BASE,                                DescriptorBase(__readfs()));
#else
    VmxVmcsWritePlatform(GUEST_FS_BASE,                                __readmsr(IA32_FS_BASE));
#endif
    VmxVmcsWrite32(      GUEST_FS_LIMIT,                               DescriptorLimit(__readfs()));
    VmxVmcsWrite32(      GUEST_FS_ACCESS_RIGHTS,                       DescriptorAccessRights(__readfs()));

    VmxVmcsWrite16(      GUEST_GS_SELECTOR,                            __readgs());
#ifndef _WIN64
    VmxVmcsWritePlatform(GUEST_GS_BASE,                                DescriptorBase(__readgs()));
#else
    VmxVmcsWritePlatform(GUEST_GS_BASE,                                __readmsr(IA32_GS_BASE));
#endif
    VmxVmcsWrite32(      GUEST_GS_LIMIT,                               DescriptorLimit(__readgs()));
    VmxVmcsWrite32(      GUEST_GS_ACCESS_RIGHTS,                       DescriptorAccessRights(__readgs()));

    VmxVmcsWrite16(      GUEST_LDTR_SELECTOR,                          __sldt());
    VmxVmcsWritePlatform(GUEST_LDTR_BASE,                              DescriptorBase(__sldt()));
    VmxVmcsWrite32(      GUEST_LDTR_LIMIT,                             DescriptorLimit(__sldt()));
    VmxVmcsWrite32(      GUEST_LDTR_ACCESS_RIGHTS,                     DescriptorAccessRights(__sldt()));

    VmxVmcsWrite16(      GUEST_TR_SELECTOR,                            __str());
    VmxVmcsWritePlatform(GUEST_TR_BASE,                                DescriptorBase(__str()));
    VmxVmcsWrite32(      GUEST_TR_LIMIT,                               DescriptorLimit(__str()));
    VmxVmcsWrite32(      GUEST_TR_ACCESS_RIGHTS,                       DescriptorAccessRights(__str()));

    VmxVmcsWritePlatform(GUEST_GDTR_BASE,                              sgdt_base());
    VmxVmcsWrite32(      GUEST_GDTR_LIMIT,                             sgdt_limit());
    VmxVmcsWritePlatform(GUEST_IDTR_BASE,                              sidt_base());
    VmxVmcsWrite32(      GUEST_IDTR_LIMIT,                             sidt_limit());

    VmxVmcsWrite64(      GUEST_IA32_DEBUGCTL,                          __readmsr(IA32_DEBUGCTL));

    VmxVmcsWrite32(      GUEST_IA32_SYSENTER_CS,                       (UINT32)  __readmsr(IA32_SYSENTER_CS));
    VmxVmcsWritePlatform(GUEST_IA32_SYSENTER_ESP,                      (UINT_PTR)__readmsr(IA32_SYSENTER_ESP));
    VmxVmcsWritePlatform(GUEST_IA32_SYSENTER_EIP,                      (UINT_PTR)__readmsr(IA32_SYSENTER_EIP));
    VmxVmcsWrite64(      GUEST_IA32_PERF_GLOBAL_CTRL,                  (UINT_PTR)__readmsr(IA32_PERF_GLOBAL_CTRL) );
    VmxVmcsWrite64(      GUEST_IA32_EFER,                              (UINT_PTR)__readmsr( IA32_MSR_EFER ) );

    //VmxVmcsWrite64(      GUEST_IA32_PAT,                               X);
    //VmxVmcsWrite64(      GUEST_IA32_BNDCFGS,                           X);
    //VmxVmcsWrite32(      GUEST_SMBASE,                                 X);
    //VmxVmcsWrite32(      GUEST_ACTIVITY_STATE,                         0);
    //VmxVmcsWrite32(      GUEST_INTERRUPTIBILITY_STATE,                 0);
    VmxVmcsWrite64(      VMCS_LINK_POINTER,                              MAXULONG_PTR );
    //VmxVmcsWritePlatform(GUEST_PENDING_DEBUG_EXCEPTIONS,               X);
    
    //VmxVmcsWrite32(      VMX_PREEMPTION_TIMER_VALUE,                   X);
    //VmxVmcsWrite64(      GUEST_PDPTE_0,                                X);
    //VmxVmcsWrite64(      GUEST_PDPTE_1,                                X);
    //VmxVmcsWrite64(      GUEST_PDPTE_2,                                X);
    //VmxVmcsWrite64(      GUEST_PDPTE_3,                                X);
    //VmxVmcsWrite16(      GUEST_INTERRUPT_STATUS,                       X);
    //VmxVmcsWrite16(      GUEST_PML_INDEX,                              X);
}

VOID
VmcsConfigureCommonHost(
    _In_ UINT_PTR SystemCr3
    )
{
    VmxVmcsWritePlatform(HOST_CR0,                                     ((__readcr0() & LOW32(__readmsr(IA32_VMX_CR0_FIXED1))) | LOW32(__readmsr(IA32_VMX_CR0_FIXED0))));
    VmxVmcsWritePlatform(HOST_CR3,                                     SystemCr3);
    VmxVmcsWritePlatform(HOST_CR4,                                     ((__readcr4() & LOW32(__readmsr(IA32_VMX_CR4_FIXED1))) | LOW32(__readmsr(IA32_VMX_CR4_FIXED0))));
    //VmxVmcsWritePlatform(HOST_RSP,                                     X);

    VmxVmcsWritePlatform( HOST_GDTR_BASE, sgdt_base() );
    VmxVmcsWritePlatform( HOST_IDTR_BASE, sidt_base() );

    VmxVmcsWrite16(      HOST_CS_SELECTOR,                             __readcs() & 0xFFFC);
    VmxVmcsWrite16(      HOST_SS_SELECTOR,                             __readss() & 0xFFFC);
    VmxVmcsWrite16(      HOST_DS_SELECTOR,                             __readds() & 0xFFFC);
    VmxVmcsWrite16(      HOST_ES_SELECTOR,                             __reades() & 0xFFFC);

    VmxVmcsWrite16(      HOST_FS_SELECTOR,                             __readfs() & 0xFFFC);
#ifndef _WIN64
    VmxVmcsWritePlatform(HOST_FS_BASE,                                 DescriptorBase(__readfs()));
#else
    VmxVmcsWritePlatform(HOST_FS_BASE,                                 __readmsr(IA32_FS_BASE));
#endif

    VmxVmcsWrite16(      HOST_GS_SELECTOR,                             __readgs() & 0xFFFC);
#ifndef _WIN64
    VmxVmcsWritePlatform(HOST_GS_BASE,                                 DescriptorBase(__readgs()));
#else
    VmxVmcsWritePlatform(HOST_GS_BASE,                                 __readmsr(IA32_GS_BASE));
#endif

    VmxVmcsWrite16(      HOST_TR_SELECTOR,                             __str() & 0xFFFC);
    VmxVmcsWritePlatform(HOST_TR_BASE,                                 DescriptorBase(__str()));

    //TODO: check if this is possible.
    VmxVmcsWrite32(      HOST_IA32_SYSENTER_CS,                        (UINT32)  __readmsr(IA32_SYSENTER_CS));
    VmxVmcsWritePlatform(HOST_IA32_SYSENTER_ESP,                       (UINT_PTR)__readmsr(IA32_SYSENTER_ESP));
    VmxVmcsWritePlatform(HOST_IA32_SYSENTER_EIP,                       (UINT_PTR)__readmsr(IA32_SYSENTER_EIP));
    //VmxVmcsWrite64(      HOST_IA32_PERF_GLOBAL_CTRL,                   X);
    //VmxVmcsWrite64(      HOST_IA32_PAT,                                X);
    //VmxVmcsWrite64(      HOST_IA32_EFER,                               X);
}

#define HMV_VPID    0x01B0

VOID
VmcsConfigureCommonControl(
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

    procPrimaryControls.Bits.SecondaryControls = 1;
    
    procSecondaryControls.Bits.enableRdtscp = 1;
    procSecondaryControls.Bits.enableXsavesXrstors = 1;
    procSecondaryControls.Bits.enableInvpcid = 1;

    //
    //  No flush of TLBs on VM entry or VM exit if VPID active. Tagged TLB
    //  entries of different virtual machines can all coexist in the TLB.
    //
    //procSecondaryControls.u.f.enableVpid = 1;

#ifdef _WIN64
    exitControls.Bits.hostAddressSpaceSize = 1;
    entryControls.Bits.ia32eModeGuest = 1;
#endif

    VmxVmcsWrite32( VM_EXEC_CONTROLS_PIN_BASED, pinControls.AsUint32);
    VmxVmcsWrite32( VM_EXEC_CONTROLS_PROC_PRIMARY, procPrimaryControls.AsUint32);
    VmxVmcsWrite32( VM_EXEC_CONTROLS_PROC_SECONDARY, procSecondaryControls.AsUint32 );
    //VmxVmcsWrite16( VM_VPID, HMV_VPID );
    VmxVmcsWrite32( VM_EXIT_CONTROLS, exitControls.AsUint32 );
    VmxVmcsWrite32( VM_ENTRY_CONTROLS, entryControls.AsUint32 );

    VmxVmcsWrite32( CR3_TARGET_COUNT, 0 );
    VmxVmcsWritePlatform( CR3_TARGET_0, 0 );
    VmxVmcsWritePlatform( CR3_TARGET_1, 0 );
    VmxVmcsWritePlatform( CR3_TARGET_2, 0 );
    VmxVmcsWritePlatform( CR3_TARGET_3, 0 );

    VmxVmcsWrite32( VM_EXIT_MSR_STORE_COUNT, 0 );
    VmxVmcsWrite32( VM_EXIT_MSR_LOAD_COUNT, 0 );
    VmxVmcsWrite32( VM_ENTRY_MSR_LOAD_COUNT, 0 );


    //VmxVmcsWrite32(      EXCEPTION_BITMAP,                             X);
    //VmxVmcsWrite32(      PAGE_FAULT_ERRORCODE_MASK,                    X);
    //VmxVmcsWrite32(      PAGE_FAULT_ERRORCODE_MATCH,                   X);
    //VmxVmcsWrite64(      IO_A_BITMAP_ADDRESS,                          X);
    //VmxVmcsWrite64(      IO_B_BITMAP_ADDRESS,                          X);
    //VmxVmcsWrite64(      TSC_OFFSET,                                   X);
    //VmxVmcsWrite64(      TSC_MULTIPLIER,                               X);
    //VmxVmcsWritePlatform(CR0_GUEST_HOST_MASK,                          X);
    //VmxVmcsWritePlatform(CR0_READ_SHADOW,                              X);
    //VmxVmcsWritePlatform(CR4_GUEST_HOST_MASK,                          X);
    //VmxVmcsWritePlatform(CR4_READ_SHADOW,                              X);
    //VmxVmcsWrite64(      APIC_ACCESS_ADDRESS,                          X);
    //VmxVmcsWrite64(      VIRTUAL_APIC_ADDRESS,                         X);
    //VmxVmcsWrite32(      TPR_THRESHOLD,                                X);
    //VmxVmcsWrite64(      EOI_EXIT_BITMAP_0,                            X);
    //VmxVmcsWrite64(      EOI_EXIT_BITMAP_1,                            X);
    //VmxVmcsWrite64(      EOI_EXIT_BITMAP_2,                            X);
    //VmxVmcsWrite64(      EOI_EXIT_BITMAP_3,                            X);
    //VmxVmcsWrite16(      POSTED_INTERRUPT_NOTIFICATION_VECTOR,         X);
    //VmxVmcsWrite64(      POSTED_INTERRUPT_DESCRIPTOR_ADDRESS,          X);
    //VmxVmcsWrite64(      EXECUTIVE_VMCS_POINTER,                       X);
    //VmxVmcsWrite64(      EPT_POINTER,                                  X);
    //VmxVmcsWrite32(      PLE_GAP,                                      X);
    //VmxVmcsWrite32(      PLE_WINDOW,                                   X);
    //VmxVmcsWrite64(      VM_FUNCTION_CONTROLS,                         X);
    //VmxVmcsWrite64(      EPTP_LIST_ADDRESS,                            X);
    //VmxVmcsWrite64(      VMREAD_BITMAP_ADDRESS,                        X);
    //VmxVmcsWrite64(      VMWRITE_BITMAP_ADDRESS,                       X);
    //VmxVmcsWrite64(      ENCLS_EXITING_BITMAP_ADDRESS,                 X);
    //VmxVmcsWrite64(      PML_ADDRESS,                                  X);
    //VmxVmcsWrite64(      VIRTUALIZATION_EXCEPTION_INFORMATION_ADDRESS, X);
    //VmxVmcsWrite16(      EPTP_INDEX,                                   X);
    //VmxVmcsWrite64(      XSS_EXITING_BITMAP,                           X);
    //VmxVmcsWrite64(      VM_EXIT_MSR_STORE_ADDRESS,                    X);
    //VmxVmcsWrite64(      VM_EXIT_MSR_LOAD_ADDRESS,                     X);
    //VmxVmcsWrite32(      VM_ENTRY_MSR_LOAD_COUNT,                      X);
    //VmxVmcsWrite64(      VM_ENTRY_MSR_LOAD_ADDRESS,                    X);
}


VOID
VmcsConfigureCommonEntry(
    _In_ UINT_PTR       rip,
    _In_ UINT_PTR       rsp,
    _In_ FLAGS_REGISTER rflags
)
{
#ifndef _WIN64
    ULONG_PTR cr4 = 0;
#endif    
    VmxVmcsWritePlatform(GUEST_RSP,    rsp);
    VmxVmcsWritePlatform(GUEST_RIP,    rip);
    VmxVmcsWritePlatform(GUEST_RFLAGS, rflags.u.raw);

    VmxVmcsWritePlatform(GUEST_CR3,    __readcr3());

#ifndef _WIN64
    cr4 = __readcr4();
    if (cr4 & CR4_PAE_ENABLED)
    {
        VMX_PROC_SECONDARY_CTLS ctls;

        ctls.AsUint32 = VmxVmcsRead32(VM_EXEC_CONTROLS_PROC_SECONDARY);

        if(ctls.Bits.enableEpt)
        {
            PHYSICAL_ADDRESS cr3;
            PVOID            page;
            PUINT64          pdptes;

            cr3.HighPart = 0;
            cr3.LowPart  = __readcr3();

            page = MmuMap(cr3);

            pdptes = (PUINT64)((UINT_PTR)page + ((__readcr3()) & 0xFF0));

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
 * IA-32 Control Register #0 (CR0)
 */
#define CR0_PE                    0x00000001
#define CR0_MP                    0x00000002
#define CR0_EM                    0x00000004
#define CR0_TS                    0x00000008
#define CR0_ET                    0x00000010
#define CR0_NE                    0x00000020
#define CR0_WP                    0x00010000
#define CR0_AM                    0x00040000
#define CR0_NW                    0x20000000
#define CR0_CD                    0x40000000
#define CR0_PG                    0x80000000

#define BITMAP_CLR64(__word, __mask) ((__word) &= ~(UINT64)(__mask))
#define BITMAP_GET64(__word, __mask) ((__word) & (UINT64)(__mask))

/*
FORCEINLINE
VOID
EnableFxOps(
    VOID
    )
{
    UINT_PTR cr0_value = __read_cr0();

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