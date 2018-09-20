#include "wdk7.h"
#include "vmcs.h"
#include "vmx.h"
#include "mmu.h"
#include "vmcsinit.h"

#define LOW32(v)  ((UINT32)(v))

typedef struct _SEGMENT_SELECTOR {
    UINT16 Selector;
    UINTN Base;
    UINT32 Limit;
    UINT32 Attr;
} SEGMENT_SELECTOR, *PSEGMENT_SELECTOR;

typedef struct _SEGMENT_CTX
 {
    SEGMENT_SELECTOR Cs;
    SEGMENT_SELECTOR Ss;
    SEGMENT_SELECTOR Ds;
    SEGMENT_SELECTOR Es;
    SEGMENT_SELECTOR Fs;
    SEGMENT_SELECTOR Gs;
    SEGMENT_SELECTOR Tr;
    SEGMENT_SELECTOR Ldtr;
} SEGMENT_CTX;

#define VM_LOAD_SEGMENT(s, SEG_NAME)                   \
{                                                       \
    VmWrite16( SEG_NAME##_SELECTOR, (s).Selector);     \
    VmWriteN( SEG_NAME##_BASE, (s).Base);               \
    VmWrite32( SEG_NAME##_LIMIT, (s).Limit);           \
    VmWrite32( SEG_NAME##_ACCESS_RIGHTS, (s).Attr);    \
}

VOID
VmcsSetGuestField(
    VOID
    )
{
    PSEGMENT_SELECTOR Seg;
    SEGMENT_CTX SegmentCtx;

    VmWriteN( GUEST_CR3, __readcr3() );
    VmWriteN( GUEST_CR0, VmcsMakeCompliantCr0( __readcr0()));
    VmWriteN( GUEST_CR4, VmcsMakeCompliantCr4( __readcr4()));
    VmWriteN( GUEST_DR7, __readdr(7));

    SegmentCtx.Cs.Selector = AsmReadCs();
    SegmentCtx.Ss.Selector = AsmReadSs();
    SegmentCtx.Ds.Selector = AsmReadDs();
    SegmentCtx.Es.Selector = AsmReadEs();
    SegmentCtx.Fs.Selector = AsmReadFs();
    SegmentCtx.Gs.Selector = AsmReadGs();
    SegmentCtx.Tr.Selector = AsmReadTr();
    SegmentCtx.Ldtr.Selector = AsmReadLdtr();

    for ( Seg = &(SegmentCtx.Cs); Seg <= &(SegmentCtx.Ldtr); Seg++ ) {

        Seg->Base = BaseFromSelector( Seg->Selector );
        Seg->Limit = AsmLimitFromSelector( Seg->Selector );
        Seg->Attr = ArFromSelector( Seg->Selector );
    }

    //
    //  Load segment selectors: cs, ss, ds, es, fs, gs, tr, ldtr.
    //
    VM_LOAD_SEGMENT( SegmentCtx.Cs, GUEST_CS );
    VM_LOAD_SEGMENT( SegmentCtx.Ss, GUEST_SS );
    VM_LOAD_SEGMENT( SegmentCtx.Ds, GUEST_DS );
    VM_LOAD_SEGMENT( SegmentCtx.Es, GUEST_ES );
    VM_LOAD_SEGMENT( SegmentCtx.Fs, GUEST_FS );
    VM_LOAD_SEGMENT( SegmentCtx.Gs, GUEST_GS );
    VM_LOAD_SEGMENT( SegmentCtx.Tr, GUEST_TR );
    VM_LOAD_SEGMENT( SegmentCtx.Ldtr, GUEST_LDTR );

#ifdef _WIN64
    VmWriteN( GUEST_FS_BASE, __readmsr( IA32_FS_BASE ) );
    VmWriteN( GUEST_GS_BASE, __readmsr( IA32_GS_BASE ) );
#endif

    VmWriteN( GUEST_GDTR_BASE, GetGdtrBase() );
    VmWrite32( GUEST_GDTR_LIMIT, GetGdtrLimit() );
    VmWriteN( GUEST_IDTR_BASE, GetIdtrBase() );
    VmWrite32( GUEST_IDTR_LIMIT, GetIdtrLimit() );

    VmWrite64( GUEST_IA32_DEBUGCTL, __readmsr( IA32_DEBUGCTL ) );

    VmWrite32( GUEST_IA32_SYSENTER_CS, (UINT32)__readmsr( IA32_SYSENTER_CS ) );
    VmWriteN( GUEST_IA32_SYSENTER_ESP, (UINTN)__readmsr( IA32_SYSENTER_ESP ) );
    VmWriteN( GUEST_IA32_SYSENTER_EIP, (UINTN)__readmsr( IA32_SYSENTER_EIP ) );
    VmWrite64( GUEST_IA32_PERF_GLOBAL_CTRL, (UINTN)__readmsr( IA32_PERF_GLOBAL_CTRL ) );
    VmWrite64( GUEST_IA32_EFER, (UINTN)__readmsr( IA32_MSR_EFER ) )
        ;
    VmWrite64( VMCS_LINK_POINTER, MAXULONG_PTR );

    VmWriteN( GUEST_PENDING_DEBUG_EXCEPTIONS, 0 );
    VmWrite32( GUEST_INTERRUPTIBILITY_STATE, 0 );
    VmWrite32( GUEST_ACTIVITY_STATE, 0 );
    VmWrite32( GUEST_SMBASE, 0);
 
    //VmWrite64(      GUEST_IA32_PAT,                               X);
    //VmWrite64(      GUEST_IA32_BNDCFGS,                           X);
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
    VmWriteN( HOST_CR0, __readcr0() );
    VmWriteN( HOST_CR4, __readcr4() );
    
    VmWrite16( HOST_CS_SELECTOR, AsmReadCs() );
    VmWrite16( HOST_SS_SELECTOR, AsmReadSs() );
    VmWrite16( HOST_DS_SELECTOR, AsmReadDs() & 0xFFF8 );
    VmWrite16( HOST_ES_SELECTOR, AsmReadEs() & 0xFFF8 );
    VmWrite16( HOST_FS_SELECTOR, AsmReadFs() );
    VmWrite16( HOST_GS_SELECTOR, AsmReadGs() );

#ifdef _WIN64
    VmWriteN( HOST_FS_BASE, __readmsr( IA32_FS_BASE ) );
    VmWriteN( HOST_GS_BASE, __readmsr( IA32_GS_BASE ) );
#else
    VmWriteN( HOST_FS_BASE, BaseFromSelector( AsmReadFs() ) );
    VmWriteN( HOST_GS_BASE, BaseFromSelector( AsmReadGs() ) );
#endif

    VmWrite16( HOST_TR_SELECTOR, AsmReadTr() );
    VmWriteN( HOST_TR_BASE, BaseFromSelector( AsmReadTr() ) );
    VmWriteN( HOST_GDTR_BASE, GetGdtrBase() );
    VmWriteN( HOST_IDTR_BASE, GetIdtrBase() );

    VmWrite32( HOST_IA32_SYSENTER_CS, (UINT32)__readmsr( IA32_SYSENTER_CS ));
    VmWriteN( HOST_IA32_SYSENTER_ESP, (UINTN)__readmsr( IA32_SYSENTER_ESP ));
    VmWriteN( HOST_IA32_SYSENTER_EIP, (UINTN)__readmsr( IA32_SYSENTER_EIP ));

    //VmWrite64( HOST_IA32_PAT, __readmsr( IA32_MSR_PAT ) );
    //VmWrite64( HOST_IA32_EFER, __readmsr( IA32_MSR_EFER ) );
}

#define VMX_XSS_EXIT_BITMAP 0UL
#define HMV_VPID            0x01B0

VOID
VmcsSetControlField(
    VOID
    )
{
    VMX_PIN_EXECUTION_CTLS PinControls = { 0 };
    VMX_PROC_PRIMARY_CTLS ProcControls = { 0 };
    VMX_PROC_SECONDARY_CTLS Proc2Controls = { 0 };
    VMX_EXIT_CTLS ExitControls = { 0 };
    VMX_ENTRY_CTLS EntryControls = { 0 };

    //
    //  TODO: enable conditionally.
    //
    //  Proc2Controls.Bits.EnableInvpcid = 1;
    //
    Proc2Controls.Bits.EnableRdtscp = 1;
    ProcControls.Bits.SecondaryControl = 1;

    if (IsXStateSupported()) {

        //
        // Expose XSAVES only when XSAVE is supported.
        //
        Proc2Controls.Bits.EnableXsavesXrstors = 1;
        //VmWrite64( XSS_EXITING_BITMAP, VMX_XSS_EXIT_BITMAP );
    }

    //
    //  TODO: enable conditionally.
    //
    //  Proc2Controls.Bits.EnableInvpcid = 1;
    //
    Proc2Controls.Bits.EnableRdtscp = 1;

    //
    //  No flush of TLBs on VM entry or VM exit if VPID active. Tagged TLB
    //  entries of different virtual machines can all coexist in the TLB.
    //
    //  Proc2Controls.u.f.enableVpid = 1;
    //

#ifdef _WIN64
    ExitControls.Bits.Ia32eHost = 1;
    EntryControls.Bits.Ia32eGuest = 1;
#endif

    VmWrite32( VM_EXEC_CONTROLS_PIN_BASED, PinControls.AsUint32);
    VmWrite32( VM_EXEC_CONTROLS_PROC_PRIMARY, ProcControls.AsUint32);
    VmWrite32( VM_EXEC_CONTROLS_PROC_SECONDARY, Proc2Controls.AsUint32 );
    VmWrite32( VM_EXIT_CONTROLS, ExitControls.AsUint32 );
    VmWrite32( VM_ENTRY_CONTROLS, EntryControls.AsUint32 );

    //VmWrite16( VM_VPID, HMV_VPID );

    VmWrite32( CR3_TARGET_COUNT, 0 );
    VmWriteN( CR3_TARGET_0, 0 );
    VmWriteN( CR3_TARGET_1, 0 );
    VmWriteN( CR3_TARGET_2, 0 );
    VmWriteN( CR3_TARGET_3, 0 );

    VmWrite32( VM_EXIT_MSR_STORE_COUNT, 0 );
    VmWrite32( VM_EXIT_MSR_LOAD_COUNT, 0 );
    VmWrite32( VM_ENTRY_MSR_LOAD_COUNT, 0 );

    VmWrite64( EXECUTIVE_VMCS_POINTER, 0 );

    //
    //  TODO: intercept Cr4 fixed bits with host and shadow masks
    //  ~(Cr4Fixed0.AsUintN ^ Cr4Fixed1.AsUintN);
    //
    //  Activate VMCS shadow:
    //  TODO: VmWrite64(      VMREAD_BITMAP_ADDRESS,                X);
    //  TODO: VmWrite64(      VMWRITE_BITMAP_ADDRESS,               X);

    //VmWrite32(      PAGE_FAULT_ERRORCODE_MASK,                    X);
    //VmWrite32(      PAGE_FAULT_ERRORCODE_MATCH,                   X);
    //VmWrite64(      IO_A_BITMAP_ADDRESS,                          X);
    //VmWrite64(      IO_B_BITMAP_ADDRESS,                          X);
    //VmWrite64(      TSC_OFFSET,                                   X);
    //VmWrite64(      TSC_MULTIPLIER,                               X);
    //VmWrite64(      APIC_ACCESS_ADDRESS,                          X);
    //VmWrite64(      VIRTUAL_APIC_ADDRESS,                         X);
    //VmWrite32(      TPR_THRESHOLD,                                X);
    //VmWrite64(      EOI_EXIT_BITMAP_0,                            X);
    //VmWrite64(      EOI_EXIT_BITMAP_1,                            X);
    //VmWrite64(      EOI_EXIT_BITMAP_2,                            X);
    //VmWrite64(      EOI_EXIT_BITMAP_3,                            X);
    //VmWrite16(      POSTED_INTERRUPT_NOTIFICATION_VECTOR,         X);
    //VmWrite64(      POSTED_INTERRUPT_DESCRIPTOR_ADDRESS,          X);
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

VOID
VmcsSetGuestPrivilegedTsd(
    VOID
    )
{
    CR4_REGISTER Cr4 = { 0 };

    //
    //  Deprivilege Tsd and make it host owned.
    //

    Cr4.AsUintN = VmReadN( GUEST_CR4 );
    Cr4.Bits.Tsd = 1;

    VmWriteN( GUEST_CR4, Cr4.AsUintN );

    //
    //  Fix: this should be additive.
    //
    VmWriteN( CR4_GUEST_HOST_MASK, (1 << 2) );
    VmWriteN( CR4_READ_SHADOW, 0 );

    //
    //  Intercept all variable= ~(fixed0 ^ fixed1);

    //
    //  Activate #GP and #UD to reinject the Tsd trigered exceptions.
    //

    VmWrite32( EXCEPTION_BITMAP, (1 << VECTOR_INVALID_OPCODE_EXCEPTION)
                               | (1 << VECTOR_GENERAL_PROTECTION_EXCEPTION) );
}

VOID
VmcsSetGuestNoMsrExits(
    _In_ PHYSICAL_ADDRESS MsrBitmap
    )
{
    VMX_PROC_PRIMARY_CTLS ProcControls;

    VmWrite64( MSR_BITMAP_ADDRESS, MsrBitmap.QuadPart );

    ProcControls.AsUint32 = VmRead32( VM_EXEC_CONTROLS_PROC_PRIMARY );
    ProcControls.Bits.UseMsrBitmap = 1;
    VmWrite32 ( VM_EXEC_CONTROLS_PROC_PRIMARY, ProcControls.AsUint32 );
}