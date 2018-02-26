#include "wdk7.h"
#include "vmcs.h"
#include "vmx.h"
#include "mmu.h"

BOOLEAN
VmcsClear(
    _In_ PVOID vmcs
)
{
    VMX_BASIC        basicVmx;
    PHYSICAL_ADDRESS vmcsPhysical;

    basicVmx.u.raw = VmxCapability(IA32_VMX_BASIC);

    *(PUINT32)vmcs = basicVmx.u.f.revisionId;

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
    //VmxVmcsWritePlatform(GUEST_RSP,                                    X);
    //VmxVmcsWritePlatform(GUEST_RIP,                                    X);
    //VmxVmcsWritePlatform(GUEST_RFLAGS,                                 X);
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
    //VmxVmcsWrite64(      GUEST_IA32_PERF_GLOBAL_CTRL,                  X);
    //VmxVmcsWrite64(      GUEST_IA32_PAT,                               X);
    //VmxVmcsWrite64(      GUEST_IA32_EFER,                              X);
    //VmxVmcsWrite64(      GUEST_IA32_BNDCFGS,                           X);
    //VmxVmcsWrite32(      GUEST_SMBASE,                                 X);
    //VmxVmcsWrite32(      GUEST_ACTIVITY_STATE,                         X);
    //VmxVmcsWrite32(      GUEST_INTERRUPTIBILITY_STATE,                 X);
    //VmxVmcsWritePlatform(GUEST_PENDING_DEBUG_EXCEPTIONS,               X);
    VmxVmcsWrite64(      VMCS_LINK_POINTER,                            (UINT64)-1);
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
    VOID
)
{
    VmxVmcsWritePlatform(HOST_CR0,                                     ((__readcr0() & LOW32(__readmsr(IA32_VMX_CR0_FIXED1))) | LOW32(__readmsr(IA32_VMX_CR0_FIXED0))));
    VmxVmcsWritePlatform(HOST_CR3,                                     __readcr3());
    VmxVmcsWritePlatform(HOST_CR4,                                     ((__readcr4() & LOW32(__readmsr(IA32_VMX_CR4_FIXED1))) | LOW32(__readmsr(IA32_VMX_CR4_FIXED0))));
    //VmxVmcsWritePlatform(HOST_RSP,                                     X);
    //VmxVmcsWritePlatform(HOST_RIP,                                     X);
    VmxVmcsWrite16(      HOST_CS_SELECTOR,                             __readcs() & 0xFFF8);
    VmxVmcsWrite16(      HOST_SS_SELECTOR,                             __readss() & 0xFFF8);
    VmxVmcsWrite16(      HOST_DS_SELECTOR,                             __readds() & 0xFFF8);
    VmxVmcsWrite16(      HOST_ES_SELECTOR,                             __reades() & 0xFFF8);
    VmxVmcsWrite16(      HOST_FS_SELECTOR,                             __readfs() & 0xFFF8);
#ifndef _WIN64
    VmxVmcsWritePlatform(HOST_FS_BASE,                                 DescriptorBase(__readfs()));
#else
    VmxVmcsWritePlatform(HOST_FS_BASE,                                 __readmsr(IA32_FS_BASE));
#endif
    VmxVmcsWrite16(      HOST_GS_SELECTOR,                             __readgs() & 0xFFF8);
#ifndef _WIN64
    VmxVmcsWritePlatform(HOST_GS_BASE,                                 DescriptorBase(__readgs()));
#else
    VmxVmcsWritePlatform(HOST_GS_BASE,                                 __readmsr(IA32_GS_BASE));
#endif
    VmxVmcsWrite16(      HOST_TR_SELECTOR,                             __str() & 0xFFF8);
    VmxVmcsWritePlatform(HOST_TR_BASE,                                 DescriptorBase(__str()));
    VmxVmcsWritePlatform(HOST_GDTR_BASE,                               sgdt_base());
    VmxVmcsWritePlatform(HOST_IDTR_BASE,                               sidt_base());
    VmxVmcsWrite32(      HOST_IA32_SYSENTER_CS,                        (UINT32)  __readmsr(IA32_SYSENTER_CS));
    VmxVmcsWritePlatform(HOST_IA32_SYSENTER_ESP,                       (UINT_PTR)__readmsr(IA32_SYSENTER_ESP));
    VmxVmcsWritePlatform(HOST_IA32_SYSENTER_EIP,                       (UINT_PTR)__readmsr(IA32_SYSENTER_EIP));
    //VmxVmcsWrite64(      HOST_IA32_PERF_GLOBAL_CTRL,                   X);
    //VmxVmcsWrite64(      HOST_IA32_PAT,                                X);
    //VmxVmcsWrite64(      HOST_IA32_EFER,                               X);
}

VOID
VmcsConfigureCommonControl(
    VOID
)
{
    VMX_PIN_CTLS            pinControls;
    VMX_PROC_PRIMARY_CTLS   procPrimaryControls;
    VMX_PROC_SECONDARY_CTLS procSecondaryControls;
    VMX_EXIT_CTLS           exitControls;
    VMX_ENTRY_CTLS          entryControls;

    pinControls.u.raw           = 0;
    procPrimaryControls.u.raw   = 0;
    procSecondaryControls.u.raw = 0;
    exitControls.u.raw          = 0;
    entryControls.u.raw         = 0;

    procPrimaryControls.u.f.activateSecondaryControls = 1;
    
    procSecondaryControls.u.f.enableRdtscp = 1;
    procSecondaryControls.u.f.enableXsavesXrstors = 1;
    procSecondaryControls.u.f.enableInvpcid = 1;

#ifdef _WIN64
    exitControls.u.f.hostAddressSpaceSize = 1;
    entryControls.u.f.ia32eModeGuest = 1;
#endif

    VmxVmcsWrite32(      VM_EXEC_CONTROLS_PIN_BASED,                   pinControls.u.raw);
    VmxVmcsWrite32(      VM_EXEC_CONTROLS_PROC_PRIMARY,                procPrimaryControls.u.raw);
    VmxVmcsWrite32(      VM_EXEC_CONTROLS_PROC_SECONDARY,              procSecondaryControls.u.raw);
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
    //VmxVmcsWrite32(      CR3_TARGET_COUNT,                             X);
    //VmxVmcsWritePlatform(CR3_TARGET_0,                                 X);
    //VmxVmcsWritePlatform(CR3_TARGET_1,                                 X);
    //VmxVmcsWritePlatform(CR3_TARGET_2,                                 X);
    //VmxVmcsWritePlatform(CR3_TARGET_3,                                 X);
    //VmxVmcsWrite64(      APIC_ACCESS_ADDRESS,                          X);
    //VmxVmcsWrite64(      VIRTUAL_APIC_ADDRESS,                         X);
    //VmxVmcsWrite32(      TPR_THRESHOLD,                                X);
    //VmxVmcsWrite64(      EOI_EXIT_BITMAP_0,                            X);
    //VmxVmcsWrite64(      EOI_EXIT_BITMAP_1,                            X);
    //VmxVmcsWrite64(      EOI_EXIT_BITMAP_2,                            X);
    //VmxVmcsWrite64(      EOI_EXIT_BITMAP_3,                            X);
    //VmxVmcsWrite16(      POSTED_INTERRUPT_NOTIFICATION_VECTOR,         X);
    //VmxVmcsWrite64(      POSTED_INTERRUPT_DESCRIPTOR_ADDRESS,          X);
    //VmxVmcsWrite64(      MSR_BITMAP_ADDRESS,                           X);
    //VmxVmcsWrite64(      EXECUTIVE_VMCS_POINTER,                       X);
    //VmxVmcsWrite64(      EPT_POINTER,                                  X);
    //VmxVmcsWrite16(      VM_VPID,                                      X);
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
    VmxVmcsWrite32(      VM_EXIT_CONTROLS,                             exitControls.u.raw);
    //VmxVmcsWrite32(      VM_EXIT_MSR_STORE_COUNT,                      X);
    //VmxVmcsWrite64(      VM_EXIT_MSR_STORE_ADDRESS,                    X);
    //VmxVmcsWrite32(      VM_EXIT_MSR_LOAD_COUNT,                       X);
    //VmxVmcsWrite64(      VM_EXIT_MSR_LOAD_ADDRESS,                     X);
    VmxVmcsWrite32(      VM_ENTRY_CONTROLS,                            entryControls.u.raw);
    //VmxVmcsWrite32(      VM_ENTRY_MSR_LOAD_COUNT,                      X);
    //VmxVmcsWrite64(      VM_ENTRY_MSR_LOAD_ADDRESS,                    X);
    //VmxVmcsWrite32(      VM_ENTRY_INTERRUPTION_INFORMATION,            X);
    //VmxVmcsWrite32(      VM_ENTRY_EXCEPTION_ERRORCODE,                 X);
    //VmxVmcsWrite32(      VM_ENTRY_INSTRUCTION_LENGTH,                  X);
}


VOID
VmcsConfigureCommonEntry(
    _In_ UINT_PTR       rip,
    _In_ UINT_PTR       rsp,
    _In_ FLAGS_REGISTER rflags
)
{
    VmxVmcsWritePlatform(GUEST_RSP,    rsp);
    VmxVmcsWritePlatform(GUEST_RIP,    rip);
    VmxVmcsWritePlatform(GUEST_RFLAGS, rflags.u.raw);

    VmxVmcsWritePlatform(GUEST_CR3,    __readcr3());
#ifndef _WIN64
    if (__readcr4() & CR4_PAE_ENABLED)
    {
        VMX_PROC_SECONDARY_CTLS ctls;

        ctls.u.raw = VmxVmcsRead32(VM_EXEC_CONTROLS_PROC_SECONDARY);

        if(ctls.u.f.enableEpt)
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
