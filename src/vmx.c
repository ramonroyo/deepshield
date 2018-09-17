#include "wdk7.h"
#include "x86.h"
#include "vmx.h"
#include "smp.h"

extern UINT8 __stdcall
AsmVmxInvEpt(
    _In_ UINTN type,
    _In_ PVOID    eptPointer
);

#define HIGH32(v64) ((UINT32)((v64) >> 32))
#define LOW32(v64)  ((UINT32) (v64)       )

const UINT16 ExceptionType[32] = {
    INTERRUPT_HARDWARE_EXCEPTION,
    INTERRUPT_HARDWARE_EXCEPTION,
    INTERRUPT_HARDWARE_EXCEPTION,
    INTERRUPT_HARDWARE_EXCEPTION,
    INTERRUPT_HARDWARE_EXCEPTION,
    INTERRUPT_HARDWARE_EXCEPTION,
    INTERRUPT_HARDWARE_EXCEPTION,
    INTERRUPT_HARDWARE_EXCEPTION,
    INTERRUPT_HARDWARE_EXCEPTION | EXCEPTION_ERROR_CODE_VALID,
    INTERRUPT_HARDWARE_EXCEPTION,
    INTERRUPT_HARDWARE_EXCEPTION | EXCEPTION_ERROR_CODE_VALID,
    INTERRUPT_HARDWARE_EXCEPTION | EXCEPTION_ERROR_CODE_VALID,
    INTERRUPT_HARDWARE_EXCEPTION | EXCEPTION_ERROR_CODE_VALID,
    INTERRUPT_HARDWARE_EXCEPTION | EXCEPTION_ERROR_CODE_VALID,
    INTERRUPT_HARDWARE_EXCEPTION | EXCEPTION_ERROR_CODE_VALID,
    INTERRUPT_HARDWARE_EXCEPTION,
    INTERRUPT_HARDWARE_EXCEPTION,
    INTERRUPT_HARDWARE_EXCEPTION | EXCEPTION_ERROR_CODE_VALID,
    INTERRUPT_HARDWARE_EXCEPTION,
    INTERRUPT_HARDWARE_EXCEPTION,
    INTERRUPT_HARDWARE_EXCEPTION,
    INTERRUPT_HARDWARE_EXCEPTION,
    INTERRUPT_HARDWARE_EXCEPTION,
    INTERRUPT_HARDWARE_EXCEPTION,
    INTERRUPT_HARDWARE_EXCEPTION,
    INTERRUPT_HARDWARE_EXCEPTION,
    INTERRUPT_HARDWARE_EXCEPTION,
    INTERRUPT_HARDWARE_EXCEPTION,
    INTERRUPT_HARDWARE_EXCEPTION,
    INTERRUPT_HARDWARE_EXCEPTION,
    INTERRUPT_HARDWARE_EXCEPTION,
    INTERRUPT_HARDWARE_EXCEPTION,
};

//
//  Compare capabilities across diferent processors.
//
typedef struct _VMX_VERIFY_INFO
{
    UINT32 RevisionId;
    UINT32 PinCtls;
    UINT32 ProcPrimaryCtls;
    UINT32 ProcSecondaryCtls;
    UINT32 ExitCtls;
    UINT32 EntryCtls;
} VMX_VERIFY_INFO, *PVMX_VERIFY_INFO;

NTSTATUS
VmxpCheckSupport(
    VOID
    )
{
    int Registers[4] = { 0 };

    //
    // Checking the CPUID on each logical processor to ensure VMX is supported and that the overall feature set of each logical processor is compatible.
    //
    __cpuid( Registers, CPUID_FEATURE_INFORMATION );

    //
    // Check ECX[5] = 1
    //
    if (!(Registers[2] & (1 << 5))) {
        return STATUS_VMX_NOT_SUPPORTED;
    }

    //
    // Check bit 2 and 0 of IA32_FEATURE_CONTROL are enabled
    //
    if ((__readmsr(IA32_FEATURE_CONTROL) & 5) != 5) {
        return STATUS_VMX_BIOS_DISABLED;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
VmxpCompareCapabilities(
    _In_ PVMX_VERIFY_INFO VerifyInfo
)
{
    VMX_MSR_BASIC VmxBasic;

    VmxBasic.AsUint64 = __readmsr(IA32_VMX_BASIC);

    //
    // Checking VMCS revision identifiers on each logical processor.
    //
    if (VerifyInfo->RevisionId != VmxBasic.Bits.RevisionId) {
        return STATUS_VMX_DIFFERENT_CONFIG_ACROSS_PROCESSORS;
    }

    //
    //  Checking each of the “allowed - 1” or “allowed - 0” fields of 
    //  the VMX capability MSR’s on each processor.
    //
    if (VerifyInfo->PinCtls != VmxAllowed(IA32_VMX_PIN_CTLS)) {
        return STATUS_VMX_DIFFERENT_CONFIG_ACROSS_PROCESSORS;
    }

    if (VerifyInfo->ProcPrimaryCtls != VmxAllowed(IA32_VMX_PROC_PRIMARY_CTLS)) {
        return STATUS_VMX_DIFFERENT_CONFIG_ACROSS_PROCESSORS;
    }

    if (VerifyInfo->ProcSecondaryCtls != VmxAllowed(IA32_VMX_PROC_SECONDARY_CTLS)) {
        return STATUS_VMX_DIFFERENT_CONFIG_ACROSS_PROCESSORS;
    }

    if (VerifyInfo->ExitCtls != VmxAllowed(IA32_VMX_EXIT_CTLS)) {
        return STATUS_VMX_DIFFERENT_CONFIG_ACROSS_PROCESSORS;
    }

    if (VerifyInfo->EntryCtls != VmxAllowed(IA32_VMX_ENTRY_CTLS)) {
        return STATUS_VMX_DIFFERENT_CONFIG_ACROSS_PROCESSORS;
    }

    if (VerifyInfo->EntryCtls != VmxAllowed(IA32_VMX_ENTRY_CTLS)) {
        return STATUS_VMX_DIFFERENT_CONFIG_ACROSS_PROCESSORS;
    }

    return STATUS_SUCCESS;
}

NTSTATUS __stdcall
VmxpVerifySupportPerCpu(
    _In_  UINT32 VcpuId,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS Status;
    PVMX_VERIFY_INFO VmxVerify;

    UNREFERENCED_PARAMETER( VcpuId );
    
    VmxVerify = (PVMX_VERIFY_INFO)Context;

    Status = VmxpCheckSupport();
    
    if (!NT_SUCCESS( Status )) {
        return Status;
    }

    Status = VmxpCompareCapabilities( VmxVerify );
    
    if (!NT_SUCCESS( Status )) {
        return Status;
    }

    return Status;
}

NTSTATUS
VmxVerifySupport(
    VOID
    )
{
    VMX_VERIFY_INFO VerifyInfo;
    VMX_MSR_BASIC VmxBasic;

    //
    //  Prepare revision and capabilities
    //
    VmxBasic.AsUint64 = __readmsr( IA32_VMX_BASIC );

    VerifyInfo.RevisionId = VmxBasic.Bits.RevisionId;
    VerifyInfo.PinCtls = VmxAllowed( IA32_VMX_PIN_CTLS );
    VerifyInfo.ProcPrimaryCtls = VmxAllowed( IA32_VMX_PROC_PRIMARY_CTLS );
    VerifyInfo.ProcSecondaryCtls = VmxAllowed( IA32_VMX_PROC_SECONDARY_CTLS );
    VerifyInfo.ExitCtls = VmxAllowed( IA32_VMX_EXIT_CTLS );
    VerifyInfo.EntryCtls = VmxAllowed( IA32_VMX_ENTRY_CTLS );
    
    return SmpRunPerProcessor( VmxpVerifySupportPerCpu, &VerifyInfo );
}

UINT64
VmxCapability(
    _In_ UINT32 Capability
    )
{
    switch (Capability)
    {
        case IA32_VMX_BASIC:
        case IA32_VMX_MISC:
            return __readmsr(Capability);

        case IA32_VMX_EPT_VPID_CAP:
        {
            VMX_PROC_PRIMARY_CTLS VmxProcPrimary;

            VmxProcPrimary.AsUint32 = VmxAllowed( IA32_VMX_PROC_PRIMARY_CTLS );

            if (VmxProcPrimary.Bits.SecondaryControl ){
                VMX_PROC_SECONDARY_CTLS VmxProcSecondary;

                VmxProcSecondary.AsUint32 = VmxAllowed( IA32_VMX_PROC_SECONDARY_CTLS );
                if (VmxProcSecondary.Bits.EnableEpt || VmxProcSecondary.Bits.EnableVpid) {
                    return __readmsr( Capability );
                }
            }
            break;
        }
    }

    return 0;
}


UINT32
VmxAllowed(
    _In_ UINT32 VmxMsr
    )
{
    switch (VmxMsr)
    {
        case IA32_VMX_PROC_SECONDARY_CTLS:
        {
            VMX_PROC_PRIMARY_CTLS VmxProcPrimary;

            VmxProcPrimary.AsUint32 = VmxAllowed(IA32_VMX_PROC_PRIMARY_CTLS);

            if (!VmxProcPrimary.Bits.SecondaryControl ) {
                return 0;
            }
        }
        case IA32_VMX_PIN_CTLS:
        case IA32_VMX_PROC_PRIMARY_CTLS:
        case IA32_VMX_EXIT_CTLS:
        case IA32_VMX_ENTRY_CTLS:
        {
            VMX_MSR_BASIC VmxBasic;
            UINT64    caps;
            UINT32    def0;
            UINT32    def1;
            UINT32    ctrl = (UINT32)-1;

            //
            // Use TRUE controls if supported
            //
            VmxBasic.AsUint64 = __readmsr(IA32_VMX_BASIC);

            if (VmxBasic.Bits.VmxControlDefaultClear 
                && (VmxMsr != IA32_VMX_PROC_SECONDARY_CTLS)) {

                //
                //  From IA32_VMX_XXXX_CTLS to IA32_VMX_TRUE_XXXX_CTLS.
                //
                VmxMsr += 0xC;
            }

            caps = __readmsr(VmxMsr);
            def0 = LOW32(caps);
            def1 = HIGH32(caps);

            return ((ctrl & def1) | def0);
        }
    }

    return 0;
}


UINT16
VmRead16(
    _In_ UINT32 field
    )
{
    UINTN data;

    if (VMCS_FIELD_WIDTH(field) != WIDTH_16) {
        return 0;
    }

    data = 0;
    if (AsmVmxRead( field, &data )) {
        return 0;
    }
    
    return (UINT16)data;
}

UINT32
VmRead32(
    _In_ UINT32 field
)
{
    UINTN data;

    if (VMCS_FIELD_WIDTH( field ) != WIDTH_32) {
        return 0;
    }

    data = 0;
    if (AsmVmxRead( field, &data )) {
        return 0;
    }

    return (UINT32)data;
}

UINT64
VmRead64(
    _In_ UINT32 field
    )
{
#ifndef _WIN64
    UINT32 low;
    UINT32 high;
#endif
    UINT64 data;

    if (VMCS_FIELD_WIDTH( field ) != WIDTH_64) {
        return 0;
    }

#ifndef _WIN64
    low = 0;
    if (AsmVmxRead( field, &low )) {
        return 0;
    }

    high = 0;
    if (AsmVmxRead( VMCS_FIELD_HIGH( field ), &high )) {
        return 0;
    }

    data  = ((UINT64)high) << 32;
    data |= low;

    return data;
#else
    data = 0;
    if (AsmVmxRead( field, &data )) {
        return 0;
    }

    return data;
#endif
}

UINTN
VmReadN(
    _In_ UINT32 field
    )
{
    UINTN data;

    if (VMCS_FIELD_WIDTH( field ) != WIDTH_PLATFORM) {
        return 0;
    }

    data = 0;
    if (AsmVmxRead( field, &data )) {
        return 0;
    }

    return data;
}

BOOLEAN
VmWrite16(
    _In_ UINT32 field,
    _In_ UINT16 data
    )
{
    UINTN toWrite;

    if (VMCS_FIELD_WIDTH( field ) != WIDTH_16) {
        return FALSE;
    }

    toWrite = (UINTN)data;
    return AsmVmxWrite( field, toWrite ) == 0;
}

static VOID 
VmxpNormalizeControls(
    _In_ UINT32  field,
    _In_ PUINT32 data
    )
{
    UINT32 VmxMsr = 0;

    switch (field)
    {
        case VM_EXEC_CONTROLS_PIN_BASED:        
            VmxMsr = IA32_VMX_PIN_CTLS;
            break;

        case VM_EXEC_CONTROLS_PROC_PRIMARY:
            VmxMsr = IA32_VMX_PROC_PRIMARY_CTLS;
            break;

        case VM_EXEC_CONTROLS_PROC_SECONDARY:
            VmxMsr = IA32_VMX_PROC_SECONDARY_CTLS;
            break;

        case VM_EXIT_CONTROLS:
            VmxMsr = IA32_VMX_EXIT_CTLS;           
            break;

        case VM_ENTRY_CONTROLS:
            VmxMsr = IA32_VMX_ENTRY_CTLS;
            break;

        default:
            return;
    }

    switch (VmxMsr)
    {
        case IA32_VMX_PROC_SECONDARY_CTLS:
        {
            VMX_PROC_PRIMARY_CTLS VmxProcPrimary;

            VmxProcPrimary.AsUint32 = VmxAllowed( IA32_VMX_PROC_PRIMARY_CTLS );
            if (!VmxProcPrimary.Bits.SecondaryControl) {
                return;
            }
        }
        case IA32_VMX_PIN_CTLS:
        case IA32_VMX_PROC_PRIMARY_CTLS:
        case IA32_VMX_EXIT_CTLS:
        case IA32_VMX_ENTRY_CTLS:
        {
            VMX_MSR_BASIC VmxBasic;
            UINT64 caps;
            UINT32 def0;
            UINT32 def1;

            VmxBasic.AsUint64 = __readmsr(IA32_VMX_BASIC);
            
            //
            //  Use TRUE controls if supported
            //
            if (VmxBasic.Bits.VmxControlDefaultClear
                && (VmxMsr != IA32_VMX_PROC_SECONDARY_CTLS)) {

                //
                //  Promote from IA32_VMX_XXXX_CTLS to IA32_VMX_TRUE_XXXX_CTLS.
                //
                VmxMsr += 0xC;
            }

            caps = __readmsr( VmxMsr );
            def0 = LOW32(caps);
            def1 = HIGH32(caps);
            *data = ((*data & def1) | def0);
        }
    }
}

BOOLEAN
VmWrite32(
    _In_ UINT32 field,
    _In_ UINT32 data
    )
{
    UINTN toWrite;

    if (VMCS_FIELD_WIDTH(field) != WIDTH_32)
        return FALSE;

    VmxpNormalizeControls(field, &data);

    toWrite = (UINTN)data;
    return AsmVmxWrite(field, toWrite) == 0;
}

BOOLEAN
VmWrite64(
    _In_ UINT32 field,
    _In_ UINT64 data
    )
{
#ifndef _WIN64
    UINT32 low;
    UINT32 high;
#endif

    if (VMCS_FIELD_WIDTH(field) != WIDTH_64)
        return 0;

#ifndef _WIN64
    low = LOW32(data);
    if (AsmVmxWrite(field, low) != 0)
        return FALSE;

    high = HIGH32(data);
    if (AsmVmxWrite(VMCS_FIELD_HIGH(field), high) != 0)
        return FALSE;

    return TRUE;
#else
    return AsmVmxWrite(field, data) == 0; 
#endif
}

BOOLEAN
VmWriteN(
    _In_ UINT32   field,
    _In_ UINTN data
)
{
    if (VMCS_FIELD_WIDTH(field) != WIDTH_PLATFORM) {
        return 0;
    }

    return AsmVmxWrite(field, data) == 0;
}

UINT8 __stdcall
VmxInvEpt(
    _In_ UINTN type,
    _In_ PVOID    eptPointer
)
{
    UINT64 entry;
    UINT64 memoryDescriptor[2];
    
    entry = *(PUINT64)eptPointer;

    memoryDescriptor[0] = entry;
    memoryDescriptor[1] = 0;
    
    return AsmVmxInvEpt(type, memoryDescriptor);
}

BOOLEAN 
IsErrorCodeRequired( 
    _In_ VMX_VECTOR_EXCEPTION Vector
    )
{
    switch (Vector) {

        case VECTOR_DOUBLE_FAULT_EXCEPTION:
        case VECTOR_INVALID_TSS_EXCEPTION:
        case VECTOR_SEGMENT_NOT_PRESENT:
        case VECTOR_STACK_FAULT_EXCEPTION:
        case VECTOR_GENERAL_PROTECTION_EXCEPTION:
        case VECTOR_PAGE_FAULT_EXCEPTION:
        case VECTOR_ALIGNMENT_CHECK_EXCEPTION:
            return TRUE;

        default:
            return FALSE;
    }
}

VOID
InjectUdException(
    VOID
    )
{
    VMX_EXIT_INTERRUPT_INFO InterruptInfo = { 0 };

    InterruptInfo.Bits.Vector = VECTOR_INVALID_OPCODE_EXCEPTION;
    InterruptInfo.Bits.InterruptType = INTERRUPT_HARDWARE_EXCEPTION;
    InterruptInfo.Bits.ErrorCodeValid = 0;
    InterruptInfo.Bits.Valid = 1;

    InjectHardwareException( InterruptInfo );
}

VOID
InjectGpException(
    _In_ UINT32 ErrorCode
    )
{
    VMX_VECTOR_EXCEPTION Vector = VECTOR_GENERAL_PROTECTION_EXCEPTION;

    VmWrite32( VM_ENTRY_EXCEPTION_ERRORCODE, ErrorCode );
    VmWrite32( VM_ENTRY_INTERRUPTION_INFORMATION,
               VMX_INT_INFO_VALID | (ExceptionType[ Vector ] << 8) | Vector );

    VmWrite32( VM_ENTRY_INSTRUCTION_LENGTH, 0 );
}

VOID
InjectHardwareException(
    _In_ VMX_EXIT_INTERRUPT_INFO InterruptInfo
    )
{
    VMX_VECTOR_EXCEPTION Vector = InterruptInfo.Bits.Vector;

    NT_ASSERT( InterruptInfo.Bits.InterruptType == INTERRUPT_HARDWARE_EXCEPTION );

    if (InterruptInfo.Bits.ErrorCodeValid) {

        NT_ASSERT( ExceptionType[Vector] & EXCEPTION_ERROR_CODE_VALID );
        NT_ASSERT( IsErrorCodeRequired( Vector ) );

        VmWrite32( VM_ENTRY_EXCEPTION_ERRORCODE, 
                   VmRead32( EXIT_INTERRUPTION_ERRORCODE ) );
    }

    VmWrite32( VM_ENTRY_INTERRUPTION_INFORMATION, 
               VMX_INT_INFO_VALID | (ExceptionType[ Vector ] << 8) | Vector );

    VmWrite32( VM_ENTRY_INSTRUCTION_LENGTH, 0 );
}

VOID
InjectInterruptOrException(
    _In_ VMX_EXIT_INTERRUPT_INFO InterruptInfo
    )
{
    UINT32 InterruptType = InterruptInfo.Bits.InterruptType;
    VM_ENTRY_CONTROL_INTERRUPT InterruptControl;

    InterruptControl.Bits.Vector = InterruptInfo.Bits.Vector;
    InterruptControl.Bits.InterruptType = InterruptInfo.Bits.InterruptType;
    InterruptControl.Bits.DeliverErrorCode = InterruptInfo.Bits.ErrorCodeValid;
    InterruptControl.Bits.Valid = InterruptInfo.Bits.Valid;

    if (InterruptControl.Bits.DeliverErrorCode) {
        VmWrite32( VM_ENTRY_EXCEPTION_ERRORCODE, 
                    VmRead32( EXIT_INTERRUPTION_ERRORCODE ) );
    }
     
    VmWrite32( VM_ENTRY_INTERRUPTION_INFORMATION, InterruptControl.AsUint32 );

    if ((InterruptType == INTERRUPT_SOFTWARE_INTERRUPT) ||
        (InterruptType == INTERRUPT_PRIVILIGED_EXCEPTION) ||
        (InterruptType == INTERRUPT_SOFTWARE_EXCEPTION)) {

        VmWrite32( VM_ENTRY_INSTRUCTION_LENGTH,
                    VmRead32( EXIT_INSTRUCTION_LENGTH ) );
    }
    else {
        VmWrite32( VM_ENTRY_INSTRUCTION_LENGTH, 0 );
    }
}

BOOLEAN
IsXStateSupported(
    VOID
    )
{
    CPU_INFO CpuInfo = { 0 };

    __cpuid( &CpuInfo, CPUID_FEATURE_INFORMATION );

    if ((CPUID_VALUE_ECX( CpuInfo ) & IA32_CPUID_ECX_XSAVE) &&
        (CPUID_VALUE_ECX( CpuInfo ) & IA32_CPUID_ECX_OSXSAVE)) {
       return TRUE;
    }

    return FALSE;
}

BOOLEAN
IsXStateEnabled(
    VOID
    )
{
    //
    //  Return if OS supports the use of XSETBV, XSAVE and XRSTOR instructions.
    //
    if (VmReadN( GUEST_CR4 ) & CR4_OSXSAVE) {
        return TRUE;
    }

    return FALSE;
}