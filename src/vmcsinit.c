#include "wdk7.h"
#include "vmcs.h"
#include "vmx.h"
#include "mem.h"
#include "vmcsinit.h"

#define MASK_PE_PG_OFF_UNRESTRICTED_GUEST 0xFFFFFFFF7FFFFFFE

#define MEMORY_INVALID         0
#define MEMORY_UNCACHED        1
#define MEMORY_WRITE_BACK      2

VOID
VmInitializeVmState(
    _Inout_ PVMX_STATE VmState
    )
{ 
    RtlZeroMemory( VmState, sizeof( VMX_STATE ));
    
    //
    //  No matter what people tell you, a small group of thoughtful initializations
    //  can change the world. This is an example.
    //

    VmState->Capabilities.Basic.AsUint64 = __readmsr( IA32_MSR_VMX_BASIC_INDEX );
    NT_ASSERT( VMCS_ABOVE_4G_SUPPORTED );
    
    VmState->Capabilities.PinBasedControls.AsUint64 = __readmsr(IA32_MSR_PIN_BASED_VM_EXECUTION_CONTROLS_INDEX);
    VmState->Capabilities.ProcessorControls.AsUint64 = __readmsr(IA32_MSR_PROCESSOR_BASED_VM_EXECUTION_CONTROLS_INDEX);
    
    VmState->Capabilities.EptVpidCapabilities.AsUint64 = 0;
    VmState->Capabilities.VmFuncControls.AsUint64 = 0;

    if (VmState->Capabilities.ProcessorControls.Bits.Maybe1.Bits.SecondaryControl ) {

        VmState->Capabilities.ProcessorControls2.AsUint64 = __readmsr( IA32_MSR_PROCESSOR_BASED_VM_EXECUTION_CONTROLS2_INDEX );

        if (VmState->Capabilities.ProcessorControls2.Bits.Maybe1.Bits.EnableEpt
            || VmState->Capabilities.ProcessorControls2.Bits.Maybe1.Bits.EnableVpid) {
            VmState->Capabilities.EptVpidCapabilities.AsUint64 = __readmsr(IA32_MSR_EPT_VPID_CAP_INDEX);
        }

        if ( VmState->Capabilities.ProcessorControls2.Bits.Maybe1.Bits.EnableVmFunctions ) {
            VmState->Capabilities.VmFuncControls.AsUint64 = __readmsr( IA32_MSR_VMX_VMFUNC_CTRL );
        }
    }

    VmState->Capabilities.ExitControl.AsUint64 = __readmsr(IA32_MSR_VM_EXIT_CONTROLS_INDEX);
    VmState->Capabilities.EntryControls.AsUint64 = __readmsr(IA32_MSR_VM_ENTRY_CONTROLS_INDEX);
    VmState->Capabilities.MiscellaneousData.AsUint64 = __readmsr(IA32_MSR_MISCELLANEOUS_DATA_INDEX);

    //
    //  If bit X is 1 in IA32_MSR_CRX_ALLOWED_ZERO_INDEX, then that bit of CRX
    //  is fixed to 1. If bit X is 0 in IA32_MSR_CRX_ALLOWED_ONE_INDEX, then
    //  that bit of CRX is fixed to 0. It is always the case that, if bit X is
    //  1 in IA32_MSR_CRX_ALLOWED_ZERO_INDEX, then that bit is also 1 in 
    //  IA32_MSR_CRX_ALLOWED_ONE_INDEX; if bit X is 0 in IA32_MSR_CRX_ALLOWED_ONE_INDEX,
    //  then that bit is also 0 in IA32_MSR_CRX_ALLOWED_ZERO_INDEX. Thus, each
    //  bit in CRX is either fixed to 0 (with Value 0 in both MSRs), fixed to 1
    //  (1 in both MSRs), or flexible (0 in IA32_MSR_CRX_ALLOWED_ZERO_INDEX and
    //  1 in IA32_MSR_CRX_ALLOWED_ONE_INDEX).
    //

    VmState->Capabilities.Cr0Maybe0.AsUintN = (UINTN)__readmsr(IA32_MSR_CR0_ALLOWED_ZERO_INDEX);
    VmState->Capabilities.Cr0Maybe1.AsUintN = (UINTN)__readmsr(IA32_MSR_CR0_ALLOWED_ONE_INDEX);
    VmState->Capabilities.Cr4Maybe0.AsUintN = (UINTN)__readmsr(IA32_MSR_CR4_ALLOWED_ZERO_INDEX);
    VmState->Capabilities.Cr4Maybe1.AsUintN = (UINTN)__readmsr(IA32_MSR_CR4_ALLOWED_ONE_INDEX);


    VmState->Constraints.PinBasedControlsMaybe1.AsUint32 = VmState->Capabilities.PinBasedControls.Bits.Maybe1.AsUint32;
    VmState->Constraints.PinBasedControlsMaybe0.AsUint32 = VmState->Capabilities.PinBasedControls.Bits.Maybe0.AsUint32;
    VmState->Constraints.ProcessorControlsMaybe1.AsUint32 = VmState->Capabilities.ProcessorControls.Bits.Maybe1.AsUint32;
    VmState->Constraints.ProcessorControlsMaybe0.AsUint32 = VmState->Capabilities.ProcessorControls.Bits.Maybe0.AsUint32;
    VmState->Constraints.ProcessorControls2Maybe1.AsUint32 = VmState->Capabilities.ProcessorControls2.Bits.Maybe1.AsUint32;
    VmState->Constraints.ProcessorControls2Maybe0.AsUint32 = VmState->Capabilities.ProcessorControls2.Bits.Maybe0.AsUint32;
    VmState->Constraints.ExitControlMaybe1.AsUint32 = VmState->Capabilities.ExitControl.Bits.Maybe1.AsUint32;
    VmState->Constraints.ExitControlMaybe0.AsUint32 = VmState->Capabilities.ExitControl.Bits.Maybe0.AsUint32;
    VmState->Constraints.EntryControlsMaybe1.AsUint32 = VmState->Capabilities.EntryControls.Bits.Maybe1.AsUint32;
    VmState->Constraints.EntryControlsMaybe0.AsUint32 = VmState->Capabilities.EntryControls.Bits.Maybe0.AsUint32;

    VmState->Constraints.Cr0Maybe1.AsUintN = VmState->Capabilities.Cr0Maybe1.AsUintN;
    VmState->Constraints.Cr0Maybe0.AsUintN = VmState->Capabilities.Cr0Maybe0.AsUintN;
    VmState->Constraints.Cr4Maybe1.AsUintN = VmState->Capabilities.Cr4Maybe1.AsUintN;
    VmState->Constraints.Cr4Maybe0.AsUintN = VmState->Capabilities.Cr4Maybe0.AsUintN;

    VmState->Constraints.VmcsRevision = VmState->Capabilities.Basic.Bits.RevisionId;
    VmState->Constraints.NumberOfCr3TargetValues = VmState->Capabilities.MiscellaneousData.Bits.NumberOfCr3TargetValue;
    VmState->Constraints.MaxMsrListsSizeInBytes =  (VmState->Capabilities.MiscellaneousData.Bits.MaxNumberOfMsrInStoreList + 1) * 512;
    VmState->Constraints.VmxTimerLength = VmState->Capabilities.MiscellaneousData.Bits.VmxPreemptionTimerRate;

    VmState->Constraints.MsegRevisionId = VmState->Capabilities.MiscellaneousData.Bits.MsegRevisionId;
    VmState->Constraints.VmEntryInHaltStateSupported = (BOOLEAN)VmState->Capabilities.MiscellaneousData.Bits.ActivityStateSupportHlt;
    VmState->Constraints.VmEntryInShutdownStateSupported = (BOOLEAN)VmState->Capabilities.MiscellaneousData.Bits.ActivityStateSupportShutdown;
    VmState->Constraints.VmEntryInWaitForSipiStateSupported = (BOOLEAN)VmState->Capabilities.MiscellaneousData.Bits.ActivityStateSupportWaitForSipi;
    VmState->Constraints.ProcessorBasedExecCtrl2Supported = (BOOLEAN)VmState->Capabilities.ProcessorControls.Bits.Maybe1.Bits.SecondaryControl;
    
    if (VmState->Constraints.ProcessorBasedExecCtrl2Supported) {
        VmState->Constraints.EptSupported = (BOOLEAN)VmState->Capabilities.ProcessorControls2.Bits.Maybe1.Bits.EnableEpt;
        VmState->Constraints.UnrestrictedGuestSupported = (BOOLEAN)VmState->Capabilities.ProcessorControls2.Bits.Maybe1.Bits.UnrestrictedGuest;
        VmState->Constraints.VpidSupported = (BOOLEAN)VmState->Capabilities.ProcessorControls2.Bits.Maybe1.Bits.EnableVpid;
        VmState->Constraints.VmfuncSupported = (BOOLEAN)VmState->Capabilities.ProcessorControls2.Bits.Maybe1.Bits.EnableVmFunctions;
        VmState->Constraints.VeSupported = (BOOLEAN) VmState->Capabilities.ProcessorControls2.Bits.Maybe1.Bits.EptVe;
        VmState->Constraints.EptpSwitchingSupported = VmState->Constraints.VmfuncSupported && VmState->Capabilities.VmFuncControls.Bits.EptpSwitching;
    }
    
    VmState->Constraints.EptVpidCapabilities = VmState->Capabilities.EptVpidCapabilities;

    VmState->Fixed.PinControlFixed1.AsUint32 = VmState->Constraints.PinBasedControlsMaybe0.AsUint32 & VmState->Constraints.PinBasedControlsMaybe1.AsUint32;
    VmState->Fixed.PinControlFixed0.AsUint32 = VmState->Constraints.PinBasedControlsMaybe0.AsUint32 | VmState->Constraints.PinBasedControlsMaybe1.AsUint32;

    VmState->Fixed.ProcessorControlsFixed1.AsUint32 = VmState->Constraints.ProcessorControlsMaybe0.AsUint32 & VmState->Constraints.ProcessorControlsMaybe1.AsUint32;
    VmState->Fixed.ProcessorControlsFixed0.AsUint32 = VmState->Constraints.ProcessorControlsMaybe0.AsUint32 | VmState->Constraints.ProcessorControlsMaybe1.AsUint32;

    VmState->Fixed.ProcessorControls2Fixed1.AsUint32 = VmState->Constraints.ProcessorControls2Maybe0.AsUint32 & VmState->Constraints.ProcessorControls2Maybe1.AsUint32;
    VmState->Fixed.ProcessorControls2Fixed0.AsUint32 = VmState->Constraints.ProcessorControls2Maybe0.AsUint32 | VmState->Constraints.ProcessorControls2Maybe1.AsUint32;

    VmState->Fixed.ExitControlFixed1.AsUint32 = VmState->Constraints.ExitControlMaybe0.AsUint32 & VmState->Constraints.ExitControlMaybe1.AsUint32;
    VmState->Fixed.ExitControlFixed0.AsUint32 = VmState->Constraints.ExitControlMaybe0.AsUint32 | VmState->Constraints.ExitControlMaybe1.AsUint32;

    VmState->Fixed.EntryControlFixed1.AsUint32 = VmState->Constraints.EntryControlsMaybe0.AsUint32 & VmState->Constraints.EntryControlsMaybe1.AsUint32;
    VmState->Fixed.EntryControlFixed0.AsUint32 = VmState->Constraints.EntryControlsMaybe0.AsUint32 | VmState->Constraints.EntryControlsMaybe1.AsUint32;

    VmState->Fixed.Cr0Fixed1.AsUintN = VmState->Constraints.Cr0Maybe0.AsUintN & VmState->Constraints.Cr0Maybe1.AsUintN;

    if (VmState->Constraints.UnrestrictedGuestSupported ) {
        //
        //  If unrestricted guest is supported then Cr0Fixed1 Value should not
        //  have PG and PE.
        //

        VmState->Fixed.Cr0Fixed1.AsUintN &= MASK_PE_PG_OFF_UNRESTRICTED_GUEST;
    }
    VmState->Fixed.Cr0Fixed0.AsUintN = VmState->Constraints.Cr0Maybe0.AsUintN | VmState->Constraints.Cr0Maybe1.AsUintN;

    VmState->Fixed.Cr4Fixed1.AsUintN = VmState->Constraints.Cr4Maybe0.AsUintN & VmState->Constraints.Cr4Maybe1.AsUintN;
    VmState->Fixed.Cr4Fixed0.AsUintN = VmState->Constraints.Cr4Maybe0.AsUintN | VmState->Constraints.Cr4Maybe1.AsUintN;
}

PVOID
VmcsAllocateRegion(
    _In_ UINT32 Revision,
    _In_ UINT32 RegionSize
    )
{
    PVMX_VMCS VmcsRegion = NULL;

    VmcsRegion = MemAllocAligned( RegionSize, PAGE_SIZE );

    if (VmcsRegion) {
        VmcsRegion->Bits.RevisionIdentifier = Revision;
        VmcsRegion->Bits.ShadowIndicator = 0;

        VmcsRegion->AbortIndicator = 0;
    }

    return VmcsRegion;
}

FORCEINLINE
UINT32
GetVmcsMemoryType(
    VOID
    )
{
    switch (gVmState.Capabilities.Basic.Bits.VmcsMemoryType ) {
        case 0:
            return MEMORY_UNCACHED;
        case 6:
            return MEMORY_WRITE_BACK;
        default:
            break;
    }

    return MEMORY_INVALID;
}

UINT32
VmcsMakeCompliantPinBasedCtrl(
    _In_ UINT32 Value
    )
{
    Value &= gVmState.Fixed.PinControlFixed0.AsUint32;
    Value |= gVmState.Fixed.PinControlFixed1.AsUint32;
    return Value;
}

UINT32
VmcsMakeCompliantProcessorCtrl(
    _In_ UINT32 Value
    )
{
    Value &= gVmState.Fixed.ProcessorControlsFixed0.AsUint32;
    Value |= gVmState.Fixed.ProcessorControlsFixed1.AsUint32;
    return Value;
}

UINT32
VmcsMakeCompliantProcessorCtrl2(
    _In_ UINT32 Value
    )
{
    Value &= gVmState.Fixed.ProcessorControls2Fixed0.AsUint32;
    Value |= gVmState.Fixed.ProcessorControls2Fixed1.AsUint32;
    return Value;
}

UINT32
VmcsMakeCompliantExitCtrl(
    _In_ UINT32 Value
    )
{
    Value &= gVmState.Fixed.ExitControlFixed0.AsUint32;
    Value |= gVmState.Fixed.ExitControlFixed1.AsUint32;
    return Value;
}

UINT32
VmcsMakeCompliantEntryCtrl(
    _In_ UINT32 Value
    )
{
    Value &= gVmState.Fixed.EntryControlFixed0.AsUint32;
    Value |= gVmState.Fixed.EntryControlFixed1.AsUint32;
    return Value;
}

UINTN 
VmcsMakeCompliantCr0(
    _In_ UINTN Value
    )
{
    Value &= gVmState.Fixed.Cr0Fixed0.AsUintN;
    Value |= gVmState.Fixed.Cr0Fixed1.AsUintN;
    return Value;
}

UINTN 
VmcsMakeCompliantCr4(
    _In_ UINTN Value
    )
{
    Value &= gVmState.Fixed.Cr4Fixed0.AsUintN;
    Value |= gVmState.Fixed.Cr4Fixed1.AsUintN;
    return Value;
}

UINTN 
VmcsGetGuestVisibleCr4(
    _In_ UINTN Cr4Value
    )
{
    UINTN Mask;
    UINTN Shadow;

    Mask = VmReadN( CR4_GUEST_HOST_MASK );
    Shadow = VmReadN( CR4_READ_SHADOW );

    return (Cr4Value & ~Mask) | (Shadow & Mask);
}