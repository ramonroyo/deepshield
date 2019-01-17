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

#define HW_EXCEPTION_UD_OR_GP(_i_)                                   \
    (INTERRUPT_HARDWARE_EXCEPTION == (_i_).Bits.InterruptType        \
    &&  ((_i_).Bits.Vector == VECTOR_INVALID_OPCODE_EXCEPTION        \
      || (_i_).Bits.Vector == VECTOR_GENERAL_PROTECTION_EXCEPTION))

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
    UINT32 Function;
    UINT32 SubLeaf;

    Function = (Registers->Rax & 0xFFFFFFFF);
    SubLeaf = (Registers->Rcx & 0xFFFFFFFF);

    if (Function == CPUID_VMM_VENDOR_SIGNATURE) {
        //
        //  Return our maximum hypervisor leaf number and the vendor ID
        //  signature.
        //

        Registers->Rbx = (UINT32)'Byte';
        Registers->Rcx = (UINT32)'Heed';
        Registers->Rdx = (UINT32)'Lynx';
        Registers->Rax = 0x40000001;

        return;
    }

    if (Function == CPUID_VMM_INTERFACE_SIGNATURE) {
        //
        //  Return the value representing our vendor interface identifier.
        //

        Registers->Rax = '1#xL';
        return;
    }

    InstrCpuidEmulate( Registers );

#ifdef REMOVE_TSX_SUPPORT
    if (Function == CPUID_EXTENDED_FEATURE_FLAGS && (SubLeaf == 0)) {

        //
        //  Disable Transactional Synchronization Extensions.
        //
        Registers->Rbx &= ~CPUID_LEAF_7H_0H_EBX_RTM;
    }
#endif

    if (Function == CPUID_FEATURE_INFORMATION) {
        //
        //  This bit has been reserved by Intel & AMD for use by hypervisors
        //  to indicate their presence.
        //

        Registers->Rcx |= CPUID_LEAF_1H_ECX_HYPERVISOR_BIT;
    }

    InstrRipAdvance( Registers );
}

#define TSC_INS_LENGHT       (2)
#define TSCP_INS_LENGHT      (3)
#define LAST_BYTE_IN_PAGE    (PAGE_SIZE - 1)

#define NUMBER_PAGES_SPANNED(Va, Size) ADDRESS_AND_SIZE_TO_SPAN_PAGES(Va,Size)

typedef struct _HVM_GUEST_INSTRUCTION {
    UINT8 Opcode[TSCP_INS_LENGHT];
    UINT8 Length;
} HVM_GUEST_INSTRUCTION, *PHVM_GUEST_INSTRUCTION;

NTSTATUS
VmReadGuestInstruction(
    _In_ UINTN Cr3,
    _In_ UINTN RemoteVa,
    _Inout_ PHVM_GUEST_INSTRUCTION Ins
    )
{
    NTSTATUS Status = STATUS_NO_MEMORY;
    PHYSICAL_ADDRESS CodePa = { 0 };
    PUINT8 CodePage = NULL;
    UINT32 PageCount = NUMBER_PAGES_SPANNED( RemoteVa, TSCP_INS_LENGHT );
    UINT32 ByteOffset = BYTE_OFFSET( RemoteVa );
    UINT8 Length = 0;

    CodePa = MmuGetPhysicalAddress( Cr3, (PVOID) RemoteVa );
    if (0 == CodePa.QuadPart) {
        goto RoutineExit;
    }

    CodePage = (PUINT8)MmuMapIoPage( CodePa, FALSE );
    if (NULL == CodePage) {
        goto RoutineExit;
    }

    Ins->Opcode[Length++] = CodePage[ByteOffset];

    if (PageCount == 1) {
        Ins->Opcode[Length++] = CodePage[ByteOffset + 1];
        Ins->Opcode[Length++] = CodePage[ByteOffset + 2];

    } else {

        if (ByteOffset != LAST_BYTE_IN_PAGE) {
            Ins->Opcode[Length++] = CodePage[ByteOffset + 1];
        }

        //
        //  After using the last byte for the current page we must map the
        //  next one to get the remaining byte/s.
        //

        MmuUnmapIoPage( CodePage );
        CodePage = NULL;

        CodePa = 
            MmuGetPhysicalAddress( Cr3, (PVOID)(RemoteVa + TSCP_INS_LENGHT) );

        if (0 == CodePa.QuadPart) {
            goto RoutineExit;
        }

        CodePage = (PUINT8)MmuMapIoPage( CodePa, FALSE );
        if (NULL == CodePage) {
            goto RoutineExit;
        }

        if (ByteOffset == LAST_BYTE_IN_PAGE) {
            //
            //  The second byte might come from either page.
            //
            Ins->Opcode[Length++] = CodePage[0];
        }

        //
        //  The third byte always comes from the second page.
        //
        Ins->Opcode[Length++] = CodePage[(ByteOffset + 2) % PAGE_SIZE];
    }

RoutineExit:
    if (CodePage) {
        MmuUnmapIoPage( CodePage );
    }

    if (Length >= TSC_INS_LENGHT) {

        //
        //  Because reading two bytes is enough for RDTSC.
        //
        Status = STATUS_SUCCESS;
        Ins->Length = Length;
    }

    return Status;
}

#define IS_RDTSC_INSTRUCTION(Ins)          \
    ((Ins).Length >= TSC_INS_LENGHT &&     \
     (Ins).Opcode[0] == 0x0F &&            \
     (Ins).Opcode[1] == 0x31)

#define IS_RDTSCP_INSTRUCTION(Ins)         \
    ((Ins).Length == TSCP_INS_LENGHT &&    \
     (Ins).Opcode[0] == 0x0F &&            \
     (Ins).Opcode[1] == 0x01 &&            \
     (Ins).Opcode[2] == 0xF9)

#define PsDirectoryTableBase(Process)    *(PUINTN)((PUINT8)Process + 0x28)

#define MaskCr3(Cr3) (Cr3 & 0xFFFFFFFFFFFFF000)

BOOLEAN
DsHvmExceptionHandler(
    _In_ PVOID Local,
    _In_ PGP_REGISTERS Registers
    )
{
    NTSTATUS Status;
    UINTN Cr3 = VmReadN( GUEST_CR3 );
    HVM_GUEST_INSTRUCTION Instruction;
    UINT32 Dpl = 0;

#ifdef OPPORTUNISTIC_HOST_CR3
    UINTN HostCr3;
    UINTN DirectoryTableBase;
#endif

    //
    //  Skip exceptions originated from kernel mode.
    //
    if (!MmuIsUserModeAddress( (PVOID)Registers->Rip) ) {

        //
        //  Get current privilege level for the descriptor as CPL = SS.DPL.
        //
        Dpl = VmRead32( GUEST_CS_ACCESS_RIGHTS );
        Dpl = (Dpl >> 5) & 3;

        NT_ASSERT( Dpl == 0 );
        return FALSE;
    }

#ifdef OPPORTUNISTIC_HOST_CR3
    DirectoryTableBase = PsDirectoryTableBase( PsGetCurrentProcess() );

    HostCr3 = __readcr3();

    if (MaskCr3( DirectoryTableBase ) == MaskCr3( HostCr3 )) {

#define GetRipByte(Registers, Index) *((PUINT8)Registers->Rip + Index)

        if (GetRipByte(Registers, 0) == 0x0F && 
             GetRipByte(Registers, 1) == 0x31) {
            VmRdtscEmulate( Local, Registers );
            return TRUE;
        }

        if (GetRipByte(Registers, 0) == 0x0F 
            && GetRipByte(Registers, 1) == 0x01 
            && GetRipByte(Registers, 2) == 0xF9 ) {

            VmRdtscpEmulate( Local, Registers );
            return TRUE;
        }

        return FALSE;
    }

    //
    //  Change the next host CR3 to opportunistically exit with the same
    //  CR3 calling RDTSC/P.
    //

    VmWriteN( HOST_CR3, DirectoryTableBase );
#endif

    Status = VmReadGuestInstruction( Cr3, Registers->Rip, &Instruction );
    if (NT_SUCCESS( Status )) {

        if (IS_RDTSC_INSTRUCTION( Instruction )) {
            VmRdtscEmulate( Local, Registers );
            return TRUE;
        }

        if (IS_RDTSCP_INSTRUCTION( Instruction ) ) {
            VmRdtscpEmulate( Local, Registers );
            return TRUE;
        }
    }

    return FALSE;
}

//
//  IRQL requirements don't mean much when running in VMM context. For all
//  intents and purposes, even if you enter at PASSIVE_LEVEL, you are 
//  technically at HIGH_LEVEL, as interrupts are disabled. So you should be
//  calling almost zero Windows APIs, except if they are documented to run
//  at HIGH_LEVEL.
//
VOID 
DsHvmExitHandler(
    _In_ UINT32 ExitReason,
    _In_ PHVM_VCPU Vcpu,
    _In_ PGP_REGISTERS Registers
    )
{
    VMX_EXIT_INTERRUPT_INFO InterruptInfo;
    BOOLEAN ExceptionHandled;

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
            InterruptInfo.AsUint32 = VmRead32( EXIT_INTERRUPTION_INFORMATION );
            if (HW_EXCEPTION_UD_OR_GP ( InterruptInfo )) {

                ExceptionHandled = DsHvmExceptionHandler( Vcpu->Context,
                                                          Registers );

                if (FALSE == ExceptionHandled) {
                    VmInjectHardwareException( InterruptInfo );
                }
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
