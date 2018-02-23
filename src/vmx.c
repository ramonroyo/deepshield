#include "wdk7.h"
#include "vmx.h"
#include "smp.h"

#define HIGH32(v64) ((UINT32)((v64) >> 32))
#define LOW32(v64)  ((UINT32) (v64)       )

#define IA32_FEATURE_CONTROL    0x3A

/**
* Intel VMX config read in a given core.
* Used to compare capabilities across diferent cores.
*/
typedef struct _VMX_SUPPORT
{
    UINT32 revisionID;
    UINT32 supportedPinCtls;
    UINT32 supportedProcPrimaryCtls;
    UINT32 supportedProcSecondaryCtls;
    UINT32 supportedExitCtls;
    UINT32 supportedEntryCtls;
} VMX_SUPPORT, *PVMX_SUPPORT;


NTSTATUS
VmxpCheckSupport(
    VOID
)
{
    int regs[4];

    //
    // Checking the CPUID on each logical processor to ensure VMX is supported and that the overall feature set of each logical processor is compatible.
    //
    __cpuid(regs, 1);

    //
    // Check ECX[5] = 1
    //
    if (!(regs[2] & (1 << 5)))
        return STATUS_VMX_NOT_SUPPORTED;

    //
    // Check bit 2 and 0 of IA32_FEATURE_CONTROL are enabled
    //
    if ((__readmsr(IA32_FEATURE_CONTROL) & 5) != 5)
        return STATUS_VMX_BIOS_DISABLED;

    return STATUS_SUCCESS;
}

NTSTATUS
VmxpCompareCapabilities(
    _In_ PVMX_SUPPORT support
)
{
    VMX_BASIC vmxBasic;

    vmxBasic.u.raw = __readmsr(IA32_VMX_BASIC);

    //
    // Checking VMCS revision identifiers on each logical processor.
    //
    if (support->revisionID != vmxBasic.u.f.revisionId)
        return STATUS_VMX_DIFFERENT_CONFIG_ACROSS_CORES;

    //
    // Checking each of the “allowed - 1” or “allowed - 0” fields of the VMX capability MSR’s on each processor.
    //
    if (support->supportedPinCtls != VmxAllowed(IA32_VMX_PIN_CTLS))
        return STATUS_VMX_DIFFERENT_CONFIG_ACROSS_CORES;

    if (support->supportedProcPrimaryCtls != VmxAllowed(IA32_VMX_PROC_PRIMARY_CTLS))
        return STATUS_VMX_DIFFERENT_CONFIG_ACROSS_CORES;

    if (support->supportedProcSecondaryCtls != VmxAllowed(IA32_VMX_PROC_SECONDARY_CTLS))
        return STATUS_VMX_DIFFERENT_CONFIG_ACROSS_CORES;

    if (support->supportedExitCtls != VmxAllowed(IA32_VMX_EXIT_CTLS))
        return STATUS_VMX_DIFFERENT_CONFIG_ACROSS_CORES;

    if (support->supportedEntryCtls != VmxAllowed(IA32_VMX_ENTRY_CTLS))
        return STATUS_VMX_DIFFERENT_CONFIG_ACROSS_CORES;

    if (support->supportedEntryCtls != VmxAllowed(IA32_VMX_ENTRY_CTLS))
        return STATUS_VMX_DIFFERENT_CONFIG_ACROSS_CORES;

    return STATUS_SUCCESS;
}


NTSTATUS __stdcall
VmxpSupport(
    _In_     UINT32 core,
    _In_opt_ PVOID  context
)
{
    PVMX_SUPPORT support;
    NTSTATUS     status;

    UNREFERENCED_PARAMETER(core);
    
    support = (PVMX_SUPPORT)context;

    status = VmxpCheckSupport();
    if (!NT_SUCCESS(status))
        return status;

    status = VmxpCompareCapabilities(support);
    if (!NT_SUCCESS(status))
        return status;

    return status;
}

NTSTATUS
VmxIsSupported(
    VOID
)
{
    VMX_SUPPORT support;
    VMX_BASIC   vmxBasic;


    if (!VmxIsIntel())
        return STATUS_VMX_NOT_SUPPORTED;

    //
    // Prepare revision and capabilities
    //
    vmxBasic.u.raw = __readmsr(IA32_VMX_BASIC);

    support.revisionID                 = vmxBasic.u.f.revisionId;
    support.supportedPinCtls           = VmxAllowed(IA32_VMX_PIN_CTLS);
    support.supportedProcPrimaryCtls   = VmxAllowed(IA32_VMX_PROC_PRIMARY_CTLS);
    support.supportedProcSecondaryCtls = VmxAllowed(IA32_VMX_PROC_SECONDARY_CTLS);
    support.supportedExitCtls          = VmxAllowed(IA32_VMX_EXIT_CTLS);
    support.supportedEntryCtls         = VmxAllowed(IA32_VMX_ENTRY_CTLS);
    
    //
    // Test on every processor
    //
    return SmpExecuteOnAllCores(VmxpSupport, &support);
}

BOOLEAN
VmxIsIntel(
    VOID
)
{
    int regs[4];

    __cpuid(regs, 0);

    //
    // GenuineIntel
    //
    return regs[2] == 'letn' && regs[3] == 'Ieni' && regs[1] == 'uneG';
}



UINT64
VmxCapability(
    _In_ UINT32 capability
)
{
    switch (capability)
    {
        case IA32_VMX_BASIC:
        case IA32_VMX_MISC:
            return __readmsr(capability);

        case IA32_VMX_EPT_VPID_CAP:
        {
            VMX_PROC_PRIMARY_CTLS vmxProcPrimary;

            vmxProcPrimary.u.raw = VmxAllowed(IA32_VMX_PROC_PRIMARY_CTLS);
            if (vmxProcPrimary.u.f.activateSecondaryControls)
            {
                VMX_PROC_SECONDARY_CTLS vmxProcSecondary;

                vmxProcSecondary.u.raw = VmxAllowed(IA32_VMX_PROC_SECONDARY_CTLS);
                if(vmxProcSecondary.u.f.enableEpt || vmxProcSecondary.u.f.enableVpid)
                    return __readmsr(capability);
            }
            break;
        }
    }

    return 0;
}


UINT32
VmxAllowed(
    _In_ UINT32 control
)
{
    switch (control)
    {
        case IA32_VMX_PROC_SECONDARY_CTLS:
        {
            VMX_PROC_PRIMARY_CTLS vmxProcPrimary;

            vmxProcPrimary.u.raw = VmxAllowed(IA32_VMX_PROC_PRIMARY_CTLS);
            if(!vmxProcPrimary.u.f.activateSecondaryControls)
                return 0;
        }
        case IA32_VMX_PIN_CTLS:
        case IA32_VMX_PROC_PRIMARY_CTLS:
        case IA32_VMX_EXIT_CTLS:
        case IA32_VMX_ENTRY_CTLS:
        {
            VMX_BASIC vmxBasic;
            UINT64    caps;
            UINT32    def0;
            UINT32    def1;

            //
            // Use TRUE controls if supported
            //
            vmxBasic.u.raw = __readmsr(IA32_VMX_BASIC);
            if(vmxBasic.u.f.defaultSettings && (control != IA32_VMX_PROC_SECONDARY_CTLS))
                control += 0xC; //From IA32_VMX_XXXX_CTLS to IA32_VMX_TRUE_XXXX_CTLS

            caps = __readmsr(control);
            def0 = LOW32(caps);
            def1 = HIGH32(caps);
            return def0 ^ def1;
        }
    }

    return 0;
}


UINT16
VmxVmcsRead16(
    _In_ UINT32 field
)
{
    UINT_PTR data;

    if (VMCS_FIELD_WIDTH(field) != WIDTH_16)
        return 0;

    data = 0;
    if (VmxVmRead(field, &data))
        return 0;
    
    return (UINT16)data;
}

UINT32
VmxVmcsRead32(
    _In_ UINT32 field
)
{
    UINT_PTR data;

    if (VMCS_FIELD_WIDTH(field) != WIDTH_32)
        return 0;

    data = 0;
    if (VmxVmRead(field, &data))
        return 0;

    return (UINT32)data;
}

UINT64
VmxVmcsRead64(
    _In_ UINT32 field
)
{
#ifndef _WIN64
    UINT32 low;
    UINT32 high;
#endif
    UINT64 data;

    if (VMCS_FIELD_WIDTH(field) != WIDTH_64)
        return 0;

#ifndef _WIN64
    low = 0;
    if (VmxVmRead(field, &low))
        return 0;

    high = 0;
    if (VmxVmRead(VMCS_FIELD_HIGH(field), &high))
        return 0;

    data  = ((UINT64)high) << 32;
    data |= low;

    return data;
#else
    data = 0;
    if (VmxVmRead(field, &data))
        return 0;

    return data;
#endif
}

UINT_PTR
VmxVmcsReadPlatform(
    _In_ UINT32 field
)
{
    UINT_PTR data;

    if (VMCS_FIELD_WIDTH(field) != WIDTH_PLATFORM)
        return 0;

    data = 0;
    if (VmxVmRead(field, &data))
        return 0;

    return data;
}


BOOLEAN
VmxVmcsWrite16(
    _In_ UINT32 field,
    _In_ UINT16 data
)
{
    UINT_PTR toWrite;

    if (VMCS_FIELD_WIDTH(field) != WIDTH_16)
        return FALSE;

    toWrite = (UINT_PTR)data;
    return VmxVmWrite(field, toWrite) == 0;
}

static VOID 
VmxpNormalizeControls(
    _In_ UINT32  field,
    _In_ PUINT32 data
)
{
    UINT32 control = 0;

    switch (field)
    {
        case VM_EXEC_CONTROLS_PIN_BASED:        control = IA32_VMX_PIN_CTLS;            break;
        case VM_EXEC_CONTROLS_PROC_PRIMARY:     control = IA32_VMX_PROC_PRIMARY_CTLS;   break;
        case VM_EXEC_CONTROLS_PROC_SECONDARY:   control = IA32_VMX_PROC_SECONDARY_CTLS; break;
        case VM_EXIT_CONTROLS:                  control = IA32_VMX_EXIT_CTLS;           break;
        case VM_ENTRY_CONTROLS:                 control = IA32_VMX_ENTRY_CTLS;          break;
        default:
            return;
    }

    switch (control)
    {
        case IA32_VMX_PROC_SECONDARY_CTLS:
        {
            VMX_PROC_PRIMARY_CTLS vmxProcPrimary;

            vmxProcPrimary.u.raw = VmxAllowed(IA32_VMX_PROC_PRIMARY_CTLS);
            if (!vmxProcPrimary.u.f.activateSecondaryControls)
                return;
        }
        case IA32_VMX_PIN_CTLS:
        case IA32_VMX_PROC_PRIMARY_CTLS:
        case IA32_VMX_EXIT_CTLS:
        case IA32_VMX_ENTRY_CTLS:
        {
            VMX_BASIC vmxBasic;
            UINT64 caps;
            UINT32 def0;
            UINT32 def1;

            vmxBasic.u.raw = __readmsr(IA32_VMX_BASIC);
            
            //
            // Use TRUE controls if supported
            //
            if (vmxBasic.u.f.defaultSettings && (control != IA32_VMX_PROC_SECONDARY_CTLS))
                control += 0xC; //From IA32_VMX_XXXX_CTLS to IA32_VMX_TRUE_XXXX_CTLS

            caps = __readmsr(control);
            def0 = LOW32(caps);
            def1 = HIGH32(caps);
            *data = ((*data & def1) | def0);
        }
    }
}

BOOLEAN
VmxVmcsWrite32(
    _In_ UINT32 field,
    _In_ UINT32 data
)
{
    UINT_PTR toWrite;

    if (VMCS_FIELD_WIDTH(field) != WIDTH_32)
        return FALSE;

    VmxpNormalizeControls(field, &data);

    toWrite = (UINT_PTR)data;
    return VmxVmWrite(field, toWrite) == 0;
}

BOOLEAN
VmxVmcsWrite64(
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
    if (VmxVmWrite(field, low) != 0)
        return FALSE;

    high = HIGH32(data);
    if (VmxVmWrite(VMCS_FIELD_HIGH(field), high) != 0)
        return FALSE;

    return TRUE;
#else
    return VmxVmWrite(field, data) == 0; 
#endif
}

BOOLEAN
VmxVmcsWritePlatform(
    _In_ UINT32   field,
    _In_ UINT_PTR data
)
{
    if (VMCS_FIELD_WIDTH(field) != WIDTH_PLATFORM)
        return 0;

    return VmxVmWrite(field, data) == 0;
}

extern
UINT8 __stdcall
VmxInvEptImpl(
    _In_ UINT_PTR type,
    _In_ PVOID    eptPointer
);

UINT8 __stdcall
VmxInvEpt(
    _In_ UINT_PTR type,
    _In_ PVOID    eptPointer
)
{
    UINT64 entry;
    UINT64 memoryDescriptor[2];
    
    entry = *(PUINT64)eptPointer;

    memoryDescriptor[0] = entry;
    memoryDescriptor[1] = 0;
    
    return VmxInvEptImpl(type, memoryDescriptor);
}
