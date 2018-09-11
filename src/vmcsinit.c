#include "wdk7.h"
#include "vmcs.h"
#include "vmx.h"
#include "vmcsinit.h"

#define MASK_PE_PG_OFF_UNRESTRICTED_GUEST 0xFFFFFFFF7FFFFFFE

VMX_CAPABILITIES Capabilities;
VMX_CONSTRAINS Constraints;
VMX_VMCS_FIXED Fixed;

/*
FORCEINLINE
ULONG
GetVmcsMemoryType(
    VOID
    )
{
    switch (Capabilities.Basic.Bits.vmcsMemoryType ) {
        case 0:
            return X;
        case 6:
            return Y;
        default:
            break;
    }

    NT_ASSERT( FALSE );
    return 1;
}*/

VOID
VmcsInitializeContext(
    VOID
    )
{ 
    RtlZeroMemory( &Capabilities, sizeof( Capabilities ));
    RtlZeroMemory( &Constraints, sizeof( Constraints ));
    RtlZeroMemory( &Fixed, sizeof( Fixed ));

    //
    //  No matter what people tell you, a small group of thoughtful initializations
    //  can change the world. This is an example.
    //

    Capabilities.Basic.AsUint64 = __readmsr( IA32_MSR_VMX_BASIC_INDEX );
    
    Capabilities.PinBasedControls.AsUint64 = __readmsr(IA32_MSR_PIN_BASED_VM_EXECUTION_CONTROLS_INDEX);
    Capabilities.ProcessorControls.AsUint64 = __readmsr(IA32_MSR_PROCESSOR_BASED_VM_EXECUTION_CONTROLS_INDEX);
    
    Capabilities.EptVpidCapabilities.AsUint64 = 0;
    Capabilities.VmFuncControls.AsUint64 = 0;

    if (Capabilities.ProcessorControls.Bits.Maybe1.Bits.SecondaryControls ) {

        Capabilities.ProcessorControls2.AsUint64 = __readmsr( IA32_MSR_PROCESSOR_BASED_VM_EXECUTION_CONTROLS2_INDEX );

        if (Capabilities.ProcessorControls2.Bits.Maybe1.Bits.enableEpt
            || Capabilities.ProcessorControls2.Bits.Maybe1.Bits.enableVpid) {
            Capabilities.EptVpidCapabilities.AsUint64 = __readmsr(IA32_MSR_EPT_VPID_CAP_INDEX);
        }

        if ( Capabilities.ProcessorControls2.Bits.Maybe1.Bits.enableVmFunctions ) {
            Capabilities.VmFuncControls.AsUint64 = __readmsr( IA32_MSR_VMX_VMFUNC_CTRL );
        }
    }

    Capabilities.ExitControl.AsUint64 = __readmsr(IA32_MSR_VM_EXIT_CONTROLS_INDEX);
    Capabilities.EntryControls.AsUint64 = __readmsr(IA32_MSR_VM_ENTRY_CONTROLS_INDEX);
    Capabilities.MiscellaneousData.AsUint64 = __readmsr(IA32_MSR_MISCELLANEOUS_DATA_INDEX);

    Capabilities.cr0Maybe0.u.raw = (UINT_PTR)__readmsr(IA32_MSR_CR0_ALLOWED_ZERO_INDEX);
    Capabilities.cr0Maybe1.u.raw = (UINT_PTR)__readmsr(IA32_MSR_CR0_ALLOWED_ONE_INDEX);
    Capabilities.cr4Maybe0.u.raw = (UINT_PTR)__readmsr(IA32_MSR_CR4_ALLOWED_ZERO_INDEX);
    Capabilities.cr4Maybe1.u.raw = (UINT_PTR)__readmsr(IA32_MSR_CR4_ALLOWED_ONE_INDEX);


    Constraints.PinBasedControlsMaybe1.AsUint32 = Capabilities.PinBasedControls.Bits.Maybe1.AsUint32;
    Constraints.PinBasedControlsMaybe0.AsUint32 = Capabilities.PinBasedControls.Bits.Maybe0.AsUint32;
    Constraints.ProcessorControlsMaybe1.AsUint32 = Capabilities.ProcessorControls.Bits.Maybe1.AsUint32;
    Constraints.ProcessorControlsMaybe0.AsUint32 = Capabilities.ProcessorControls.Bits.Maybe0.AsUint32;
    Constraints.ProcessorControls2Maybe1.AsUint32 =    Capabilities.ProcessorControls2.Bits.Maybe1.AsUint32;
    Constraints.ProcessorControls2Maybe0.AsUint32 =    Capabilities.ProcessorControls2.Bits.Maybe0.AsUint32;
    Constraints.ExitControlMaybe1.AsUint32 = Capabilities.ExitControl.Bits.Maybe1.AsUint32;
    Constraints.ExitControlMaybe0.AsUint32 = Capabilities.ExitControl.Bits.Maybe0.AsUint32;
    Constraints.EntryControlsMaybe1.AsUint32 = Capabilities.EntryControls.Bits.Maybe1.AsUint32;
    Constraints.EntryControlsMaybe0.AsUint32 = Capabilities.EntryControls.Bits.Maybe0.AsUint32;

    Constraints.cr0Maybe1.u.raw = Capabilities.cr0Maybe1.u.raw;
    Constraints.cr0Maybe0.u.raw = Capabilities.cr0Maybe0.u.raw;
    Constraints.cr4Maybe1.u.raw = Capabilities.cr4Maybe1.u.raw;
    Constraints.cr4Maybe0.u.raw = Capabilities.cr4Maybe0.u.raw;

    Constraints.VmcsRevision = Capabilities.Basic.Bits.revisionId;
    Constraints.NumberOfCr3TargetValues = Capabilities.MiscellaneousData.Bits.numberOfCr3TargetValues;
    Constraints.MaxMsrListsSizeInBytes =  (Capabilities.MiscellaneousData.Bits.msrListsMaxSize + 1) * 512;
    Constraints.VmxTimerLength = Capabilities.MiscellaneousData.Bits.preemptionTimerLength;

    Constraints.MsegRevisionId = Capabilities.MiscellaneousData.Bits.msegRevisionId;
    Constraints.VmEntryInHaltStateSupported = (BOOLEAN)Capabilities.MiscellaneousData.Bits.entryInHaltStateSupported;
    Constraints.VmEntryInShutdownStateSupported = (BOOLEAN)Capabilities.MiscellaneousData.Bits.entryInShutdownStateSupported;
    Constraints.VmEntryInWaitForSipiStateSupported = (BOOLEAN)Capabilities.MiscellaneousData. Bits.entryInWaitForSipiStateSupported;
    Constraints.ProcessorBasedExecCtrl2Supported = (BOOLEAN)Capabilities.ProcessorControls.Bits.Maybe1.Bits.SecondaryControls;
    
    if (Constraints.ProcessorBasedExecCtrl2Supported) {
        Constraints.EptSupported = (BOOLEAN)Capabilities.ProcessorControls2.Bits.Maybe1.Bits.enableEpt;
        Constraints.UnrestrictedGuestSupported = (BOOLEAN)Capabilities.ProcessorControls2.Bits.Maybe1.Bits.unrestrictedGuest;
        Constraints.VpidSupported = (BOOLEAN)Capabilities.ProcessorControls2.Bits.Maybe1.Bits.enableVpid;
        Constraints.VmfuncSupported = (BOOLEAN)Capabilities.ProcessorControls2.Bits.Maybe1.Bits.enableVmFunctions;
        Constraints.VeSupported = (BOOLEAN) Capabilities.ProcessorControls2.Bits.Maybe1.Bits.eptVe;
        Constraints.EptpSwitchingSupported = Constraints.VmfuncSupported && Capabilities.VmFuncControls.Bits.eptpSwitching;
    }
    
    Constraints.EptVpidCapabilities = Capabilities.EptVpidCapabilities;

    Fixed.PinControlFixed1.AsUint32 = Constraints.PinBasedControlsMaybe0.AsUint32 & Constraints.PinBasedControlsMaybe1.AsUint32;
    Fixed.PinControlFixed0.AsUint32 = Constraints.PinBasedControlsMaybe0.AsUint32 | Constraints.PinBasedControlsMaybe1.AsUint32;

    Fixed.ProcessorControlsFixed1.AsUint32 = Constraints.ProcessorControlsMaybe0.AsUint32 & Constraints.ProcessorControlsMaybe1.AsUint32;
    Fixed.ProcessorControlsFixed0.AsUint32 = Constraints.ProcessorControlsMaybe0.AsUint32 | Constraints.ProcessorControlsMaybe1.AsUint32;

    Fixed.ProcessorControls2Fixed1.AsUint32 = Constraints.ProcessorControls2Maybe0.AsUint32 & Constraints.ProcessorControls2Maybe1.AsUint32;
    Fixed.ProcessorControls2Fixed0.AsUint32 = Constraints.ProcessorControls2Maybe0.AsUint32 | Constraints.ProcessorControls2Maybe1.AsUint32;

    Fixed.ExitControlFixed1.AsUint32 = Constraints.ExitControlMaybe0.AsUint32 & Constraints.ExitControlMaybe1.AsUint32;
    Fixed.ExitControlFixed0.AsUint32 = Constraints.ExitControlMaybe0.AsUint32 | Constraints.ExitControlMaybe1.AsUint32;

    Fixed.EntryControlFixed1.AsUint32 = Constraints.EntryControlsMaybe0.AsUint32 & Constraints.EntryControlsMaybe1.AsUint32;
    Fixed.EntryControlFixed0.AsUint32 = Constraints.EntryControlsMaybe0.AsUint32 | Constraints.EntryControlsMaybe1.AsUint32;

    Fixed.cr0Fixed1.u.raw = Constraints.cr0Maybe0.u.raw & Constraints.cr0Maybe1.u.raw;

    if (Constraints.UnrestrictedGuestSupported ) {
        //
        //  If unrestricted guest is supported then cr0Fixed1 value should not
        //  have PG and PE.
        //

        Fixed.cr0Fixed1.u.raw &= MASK_PE_PG_OFF_UNRESTRICTED_GUEST;
    }
    Fixed.cr0Fixed0.u.raw = Constraints.cr0Maybe0.u.raw | Constraints.cr0Maybe1.u.raw;

    Fixed.cr4Fixed1.u.raw = Constraints.cr4Maybe0.u.raw & Constraints.cr4Maybe1.u.raw;
    Fixed.cr4Fixed0.u.raw = Constraints.cr4Maybe0.u.raw | Constraints.cr4Maybe1.u.raw;

    //MON_DEBUG_CODE(print_vmx_capabilities());
}