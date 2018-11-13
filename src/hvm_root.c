#include "dsdef.h"
#include "hvm.h"
#include "instr.h"

PHVM_VCPU ROOT_MODE_API
HvmGetCurrentVcpu(
    _In_ PHVM Hvm
    )
{
    return &Hvm->VcpuArray[SmpGetCurrentProcessor()];
}

UINT32 ROOT_MODE_API
HvmGetVcpuId(
    _In_ PHVM_VCPU Vcpu
   )
{
    NT_ASSERT( Vcpu );
    return Vcpu->Index;
}

PHVM ROOT_MODE_API
HvmGetVcpuHvm(
    _In_ PHVM_VCPU Vcpu
    )
{
    NT_ASSERT( Vcpu );
    return Vcpu->Hvm;
}

PHOST_SAVED_STATE ROOT_MODE_API
HvmGetVcpuHostState(
    _In_ PHVM_VCPU Vcpu
    )
{
    NT_ASSERT( Vcpu );
    return &Vcpu->HostState;
}

BOOLEAN
HvmMsrHandlerRegistered(
    _In_ PGP_REGISTERS Registers
    )
{
    //
    //  TODO: implement MSR handler registration and lookup.
    //
    if (LOW32( Registers->Rcx ) == IA32_FS_BASE) {
        return TRUE;
    }

    return FALSE;
}

#define TRUST_LEVEL_NONE           (0x00000001)
#define TRUST_LEVEL_EXEMPTED       (0x00000002)
#define TRUST_LEVEL_FORCEFUL       (0x00010000)
#define TRUST_LEVEL_MASSIVE        (TRUST_LEVEL_FORCEFUL | TRUST_LEVEL_EXEMPTED)

UINT32
DsGetRequestorTrustLevel(
    VOID
    )
{
    PETHREAD CurrentThread;
    PEPROCESS CurrentProcess;

    CurrentProcess = IoGetCurrentProcess();
    CurrentThread = KeGetCurrentThread();

    // SeLocateProcessImageName, PsGetProcessImageFileName
    // PsReferenceProcessFilePointer + IoQueryFileDosDeviceName. ObQueryNameString 

    return TRUST_LEVEL_MASSIVE;
}

VOID
HvmSetNextQuantumTsd(
    BOOLEAN EnableTsd
    )
{
    CR4_REGISTER Cr4;
    UINTN ExpiredTsd;

    Cr4.AsUintN = __readcr4();
    ExpiredTsd = Cr4.AsUintN & CR4_TSD;

    if ((EnableTsd && ExpiredTsd) || (!EnableTsd && !ExpiredTsd)) {
        return;
    }

    Cr4.AsUintN = VmMakeCompliantCr4( VmGetGuestVisibleCr4( Cr4.AsUintN ) );
    Cr4.AsUintN ^= CR4_TSD;

    RtlPostMailboxTrace( &gSecureMailbox,
                         TRACE_LEVEL_INFORMATION,
                         TRACE_MSR_ROOT,
                         "New CR4 set (Cid = %4d.%4d, Cr4 = %I64X)\n",
                         PsGetCurrentProcessId(),
                         PsGetCurrentThreadId(),
                         Cr4.AsUintN );

    VmWriteN( GUEST_CR4, Cr4.AsUintN );
}

BOOLEAN
HvmHandleMsrFsBase( 
    _In_ PGP_REGISTERS Registers
    )
{
    BOOLEAN EnableTsd;
    UINT64 Value;

    Value = (((UINT64) LOW32( Registers->Rdx )) << 32) | LOW32( Registers->Rax );

    //
    //  On 32-bit mode the FS segment register points to TEB and the KPCR, but
    //  on 64-bit mode it is the GS register that points to the TEB. However
    //  when running Wow64 apps the FS register continues to point to the 32-bit
    //  version of the TEB.
    //

    RtlPostMailboxTrace( &gSecureMailbox,
                         TRACE_LEVEL_VERBOSE,
                         TRACE_MSR_ROOT,
                         "Writing MSR_FS_BASE (Cid = %4d.%4d, Teb = %I64X)\n",
                         PsGetCurrentProcessId(),
                         PsGetCurrentThreadId(),
                         Value );

    EnableTsd = !FlagOn( DsGetRequestorTrustLevel(), TRUST_LEVEL_EXEMPTED );
    HvmSetNextQuantumTsd( EnableTsd );

    return FALSE;
}

BOOLEAN ROOT_MODE_API
HvmVcpuCommonExitsHandler(
    _In_ UINT32 ExitReason,
    _In_ PHVM_VCPU Vcpu,
    _In_ PGP_REGISTERS Registers
    )
{
    BOOLEAN ExitHandled = FALSE;

    UNREFERENCED_PARAMETER(Vcpu);

    switch (ExitReason)
    {
        case EXIT_REASON_INVD:
        {
            InstrInvdEmulate( Registers );
            InstrRipAdvance( Registers );
            return TRUE;
        }
#ifdef _WIN64
        case EXIT_REASON_XSETBV:
        {
            if (InstrXsetbvEmulate( Registers )) {
                InstrRipAdvance( Registers );
            }
            return TRUE;
        }
#endif

#ifdef HVM_EMULATE_INVVPID
        case EXIT_REASON_INVVPID:
        {
            InstrInvVpidEmulate( Registers );
            InstrRipAdvance( Registers );
            return TRUE;
        }
#endif

        //
        //  Virtualization instructions.
        //
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
        {
            VmInjectUdException();

            //
            // [RR] TODO: Describe why this is necessary.
            //
            Registers->Rflags.Bits.rf = 1;

            VmWriteN( GUEST_RFLAGS, Registers->Rflags.AsUintN );
            return TRUE;
        }

        case EXIT_REASON_MSR_READ:
        {
            InstrMsrReadEmulate( Registers );
            InstrRipAdvance( Registers );
            return TRUE;
        }
        case EXIT_REASON_MSR_WRITE:
        {
            if (HvmMsrHandlerRegistered( Registers )) {
                ExitHandled = HvmHandleMsrFsBase( Registers );
            }

            if (!ExitHandled) {
                InstrMsrWriteEmulate( Registers );
                InstrRipAdvance( Registers );
            }

            return TRUE;
        }
    }

    return FALSE;
}
