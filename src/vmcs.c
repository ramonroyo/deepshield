#include "wdk7.h"
#include "vmcs.h"
#include "vmx.h"
#include "mmu.h"
#include "vmcsinit.h"

extern VOID
AsmHvmpExitHandler(
    VOID
    );

#define VM_LOAD_SEGMENT(s, SEG_NAME)                   \
{                                                      \
    VmWrite16( SEG_NAME##_SELECTOR, (s).Selector );    \
    VmWriteN( SEG_NAME##_BASE, (s).Base );             \
    VmWrite32( SEG_NAME##_LIMIT, (s).Limit );          \
    VmWrite32( SEG_NAME##_ACCESS_RIGHTS, (s).Attr );   \
}

VOID
VmcsSetGuestField(
    VOID
    )
{
    PSEGMENT_SELECTOR Seg;
    SEGMENT_CTX SegmentCtx;

    VmWriteN( GUEST_CR3, __readcr3() );
    VmWriteN( GUEST_CR0, VmMakeCompliantCr0( __readcr0()));
    VmWriteN( GUEST_CR4, VmMakeCompliantCr4( __readcr4()));
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

    VmWrite64( VMCS_LINK_POINTER, MAXULONG_PTR );
    VmWriteN( GUEST_PENDING_DEBUG_EXCEPTIONS, 0 );
    VmWrite32( GUEST_INTERRUPTIBILITY_STATE, 0 );
    VmWrite32( GUEST_ACTIVITY_STATE, 0 );
    VmWrite32( GUEST_SMBASE, 0);

    //
    //  Remove support for nested guest with CR4_VMXE.
    //
    VmWriteN( CR4_GUEST_HOST_MASK, CR4_VMXE );
    VmWriteN( CR4_READ_SHADOW, 0 );

    //
    //  TODO: intercept Cr4 variable bits with host and shadow masks
    //  ~(Cr4Fixed0.AsUintN ^ Cr4Fixed1.AsUintN);
    //

    //VmWrite64( GUEST_IA32_PERF_GLOBAL_CTRL, (UINTN)__readmsr( IA32_PERF_GLOBAL_CTRL ) );
    //VmWrite64( GUEST_IA32_EFER, (UINTN)__readmsr( IA32_MSR_EFER ) );
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
    _In_ UINTN HostCr3
    )
{
    VmWriteN( HOST_CR3, HostCr3 );
    VmWriteN( HOST_CR0, __readcr0() ); //VmMakeCompliantCr0( __readcr0() ));
    VmWriteN( HOST_CR4, __readcr4() ); //VmMakeCompliantCr4( __readcr4() ));

    VmWrite16( HOST_CS_SELECTOR, AsmReadCs() & 0xFFF8 );
    VmWrite16( HOST_SS_SELECTOR, AsmReadSs() & 0xFFF8 );
    VmWrite16( HOST_DS_SELECTOR, AsmReadDs() & 0xFFF8 );
    VmWrite16( HOST_ES_SELECTOR, AsmReadEs() & 0xFFF8 );
    VmWrite16( HOST_FS_SELECTOR, AsmReadFs() & 0xFFF8 );
    VmWrite16( HOST_GS_SELECTOR, AsmReadGs() & 0xFFF8 );
    VmWrite16( HOST_TR_SELECTOR, AsmReadTr() & 0xFFF8 );

#ifdef _WIN64
    VmWriteN( HOST_FS_BASE, __readmsr( IA32_FS_BASE ) );
    VmWriteN( HOST_GS_BASE, __readmsr( IA32_GS_BASE ) );
#else
    VmWriteN( HOST_FS_BASE, BaseFromSelector( AsmReadFs() ) );
    VmWriteN( HOST_GS_BASE, BaseFromSelector( AsmReadGs() ) );
#endif

    VmWriteN( HOST_TR_BASE, BaseFromSelector( AsmReadTr() ) );
    VmWriteN( HOST_GDTR_BASE, GetGdtrBase() );
    VmWriteN( HOST_IDTR_BASE, GetIdtrBase() );

    VmWrite32( HOST_IA32_SYSENTER_CS, (UINT32)__readmsr( IA32_SYSENTER_CS ));
    VmWriteN( HOST_IA32_SYSENTER_ESP, (UINTN)__readmsr( IA32_SYSENTER_ESP ));
    VmWriteN( HOST_IA32_SYSENTER_EIP, (UINTN)__readmsr( IA32_SYSENTER_EIP ));

    //VmWrite64( HOST_IA32_PAT, __readmsr( IA32_MSR_PAT ) );
    //VmWrite64( HOST_IA32_EFER, __readmsr( IA32_MSR_EFER ) );
}

#define VMX_XSS_EXIT_BITMAP 0ULL
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

    if (IsXStateSupported()) {

        //
        // Expose XSAVES only if XSAVE is supported.
        //
        Proc2Controls.Bits.EnableXsavesXrstors = 1;
        VmWrite64( XSS_EXITING_BITMAP, VMX_XSS_EXIT_BITMAP );
    }

    Proc2Controls.Bits.EnableInvpcid = IsInvpcidSupported();
    Proc2Controls.Bits.EnableRdtscp = IsRdtscpSupported();

    ProcControls.Bits.SecondaryControl = 1;

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
    //  TODO: activate VMCS shadow.
    //
    //  VmWrite64(      VMREAD_BITMAP_ADDRESS,                X);
    //  VmWrite64(      VMWRITE_BITMAP_ADDRESS,               X);
    //
    //  TODO: research use of VPID
    //  VmWrite16( VM_VPID, HMV_VPID );
    //
}

VOID
VmcsSetCommonField(
    _In_ UINTN HostRsp,
    _In_ UINTN Rip,
    _In_ UINTN Rsp,
    _In_ UINTN Flags
    )
{
    FLAGS_REGISTER RegFlags;
    RegFlags.AsUintN = Flags;

    VmWriteN( HOST_RSP, HostRsp );
    VmWriteN( HOST_RIP, (UINTN) AsmHvmpExitHandler );

    //
    //  Reserved bits 63:22 (bits 31:22 on processors that do not support
    //  Intel 64 architecture), bit 15, bit 5 and bit 3 must be 0 in the field,
    //  and reserved bit 1 must be 1.
    //
    //  The VM flag (bit 17) must be 0 either if the “IA - 32e mode guest”
    //  entry control is 1 or if CR0.PE is 0.
    //

    RegFlags.Bits.Cf = 0;
    RegFlags.Bits.Zf = 0;

    VmWriteN( GUEST_RSP, Rsp );
    VmWriteN( GUEST_RIP, Rip );
    VmWriteN( GUEST_RFLAGS, RegFlags.AsUintN );
}

VOID
VmSetPrivilegedTimeStamp(
    VOID
    )
{
    UINTN CurrentTsd = VmReadN( GUEST_CR4 ) & CR4_TSD;
    
    //
    //  Deprivilege TSD and make the bit host owned.
    //
    VmWriteN( GUEST_CR4, VmReadN( GUEST_CR4 ) | CR4_TSD );
    VmWriteN( CR4_GUEST_HOST_MASK, VmReadN( CR4_GUEST_HOST_MASK ) | CR4_TSD );

    //
    //  Let the guest read what was set previously.
    //
    VmWriteN( CR4_READ_SHADOW, VmReadN( CR4_READ_SHADOW ) & CurrentTsd );

    //
    //  Activate #GP and #UD to reinject the TSD trigered exceptions.
    //
    VmWrite32( EXCEPTION_BITMAP, (1 << VECTOR_INVALID_OPCODE_EXCEPTION)
                               | (1 << VECTOR_GENERAL_PROTECTION_EXCEPTION) );
}

NTSTATUS
VmSetMsrBitsConfig(
    _Inout_ PUINT8 MsrBitmap,
    _In_ UINT32 MsrId,
    _In_ MSR_ACCESS Access,
    _In_ BOOLEAN Set
    )
{
    UINT32 BitNumber;
    MSR_ACCESS AccessIdx;
    PUINT8 BitArray;

    for (AccessIdx = WRITE_ACCESS; AccessIdx <= READ_ACCESS; ++AccessIdx ) {

        if (AccessIdx & Access) {
            
            if (MsrId <= MSR_LOW_LAST) {

                BitNumber = MsrId;
                BitArray = (READ_ACCESS == AccessIdx) ?
                         &MsrBitmap[MSR_READ_LOW_OFFSET] :
                         &MsrBitmap[MSR_WRITE_LOW_OFFSET];

            } else if (MSR_HIGH_FIRST <= MsrId && MsrId <= MSR_HIGH_LAST) {

                BitNumber = MsrId - MSR_HIGH_FIRST;
                BitArray = (READ_ACCESS == AccessIdx) ?
                         &MsrBitmap[MSR_READ_HIGH_OFFSET] :
                         &MsrBitmap[MSR_WRITE_HIGH_OFFSET];

            } else {
                NT_ASSERT( FALSE );
                return STATUS_NOT_FOUND;
            }

            if (Set) {
                BITARRAY_SET( BitArray, BitNumber );
            } else {
                BITARRAY_CLR( BitArray, BitNumber );
            }
        }
    }
    return STATUS_SUCCESS;
}

VOID
VmcsSetGuestMsrExitPolicy(
    _In_ PUCHAR MsrBitmap
    )
{
    NTSTATUS Status;
    VMX_PROC_PRIMARY_CTLS ProcControls;
    PHYSICAL_ADDRESS MsrBitmapHpa;

    //
    //  There are two methods to update the contents of the FS_BASE and GS_BASE
    //  hidden descriptor fields. The first is available exclusively to CPL = 0
    //  where the register fields are mapped to MSRs. Privileged software can
    //  load a 64-bit base address in canonical form into FS_BASE or GS_BASE
    //  using a single WRMSR instruction. The second method of updating the FS
    //  and GS base register fields is available to software running at any
    //  privilege level when supported by the implementation and enabled by
    //  setting CR4[FSGSBASE]. The WRFSBASE and WRGSBASE instructions copy the
    //  contents of a GPR to the FS_BASE and GS_BASE fields respectively. Note
    //  that WRFSBASE and WRGSBASE are only supported in 64-bit mode.
    //
    //  A couple of major benefits are expected from the FSGSBASE instruction
    //  set introduced from Ivy Bridge:
    //  
    //    1) Performance improvements in context switches by skipping MSR
    //       writes for FS and GS base.
    //
    //    2) UMS availability. UMS efficiently allows user-mode processes to
    //       switch between multiple “user” threads, hence FSGSBASE is enforced
    //       to switch the TEB without involving a kernel transition that would
    //       defy the whole point.
    //
    //  Set MsrBitmap exiting for IA32_FS_BASE.
    //
    Status = VmSetMsrBitsConfig( MsrBitmap, IA32_FS_BASE, WRITE_ACCESS, TRUE );
    NT_ASSERT( STATUS_SUCCESS == Status );

    MsrBitmapHpa = MmuGetPhysicalAddress( 0, MsrBitmap );
    VmWrite64( MSR_BITMAP_ADDRESS, MsrBitmapHpa.QuadPart );

    ProcControls.AsUint32 = VmRead32( VM_EXEC_CONTROLS_PROC_PRIMARY );
    ProcControls.Bits.UseMsrBitmap = 1;
    VmWrite32( VM_EXEC_CONTROLS_PROC_PRIMARY, ProcControls.AsUint32 );
}