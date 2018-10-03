#ifndef _VMCSINIT_
#define _VMCSINIT_

#include <ntifs.h>
#include "vmx.h"
#include "x86.h"

#define VMCS_REGION_SIZE                                       \
    (UINT32)((gVmState.Capabilities).Basic.Bits.VmcsRegionSize)

#define VMCS_REVISION                                          \
    (UINT32)((gVmState.Capabilities).Basic.Bits.RevisionId)

#define VMCS_ABOVE_4G_SUPPORTED                                \
    (UINT32)((gVmState.Capabilities).Basic.Bits.VmcsAddrWidth  == 0)

//
//  VMX MSR Indexes.
//
#define IA32_MSR_OPT_IN_INDEX                                 0x3A
#define IA32_MSR_MSEG_INDEX                                   0x9B
#define IA32_MSR_VMX_BASIC_INDEX                              0x480
#define IA32_MSR_PIN_BASED_VM_EXECUTION_CONTROLS_INDEX        0x481
#define IA32_MSR_PROCESSOR_BASED_VM_EXECUTION_CONTROLS_INDEX  0x482
#define IA32_MSR_PROCESSOR_BASED_VM_EXECUTION_CONTROLS2_INDEX 0x48B
#define IA32_MSR_VM_EXIT_CONTROLS_INDEX                       0x483
#define IA32_MSR_VM_ENTRY_CONTROLS_INDEX                      0x484
#define IA32_MSR_MISCELLANEOUS_DATA_INDEX                     0x485
#define IA32_MSR_CR0_ALLOWED_ZERO_INDEX                       0x486
#define IA32_MSR_CR0_ALLOWED_ONE_INDEX                        0x487
#define IA32_MSR_CR4_ALLOWED_ZERO_INDEX                       0x488
#define IA32_MSR_CR4_ALLOWED_ONE_INDEX                        0x489
#define IA32_MSR_VMX_VMCS_ENUM                                0x48A
#define IA32_MSR_EPT_VPID_CAP_INDEX                           0x48C
#define IA32_MSR_TRUE_PINBASED_CTLS_INDEX                     0x48D
#define IA32_MSR_TRUE_PROCBASED_CTLS_INDEX                    0x48E
#define IA32_MSR_TRUE_EXIT_CTLS_INDEX                         0x48F
#define IA32_MSR_TRUE_ENTRY_CTLS_INDEX                        0x490
#define IA32_MSR_VMX_VMFUNC_CTRL                              0x491
#define IA32_MSR_VMX_FIRST                                    0x480
#define IA32_MSR_VMX_LAST                                     0x491


//
//  VMCS Data Structure.
//
typedef struct _VMX_VMCS {

    struct
    {
        unsigned RevisionIdentifier : 31;
        unsigned ShadowIndicator    : 1;
    } Bits;

    UINT32 AbortIndicator;
}  VMX_VMCS, *PVMX_VMCS;

//
//  Contains the complete set of VMX MSR Values.
//
typedef struct _VMX_CAPABILITIES {
    VMX_MSR_BASIC Basic;
    VMX_PIN_EXECUTION_CTLS_MAYBE PinBasedControls;
    VMX_PROC_PRIMARY_CTLS_MAYBE ProcessorControls;
    VMX_PROC_SECONDARY_CTLS_MAYBE ProcessorControls2;
    VMX_EXIT_CTLS_MAYBE ExitControl;
    VMX_ENTRY_CTLS_MAYBE EntryControls;
    VMX_MISC_DATA MiscellaneousData;
    VMX_MSR_CR0 Cr0Maybe0;
    VMX_MSR_CR0 Cr0Maybe1;
    VMX_MSR_CR4 Cr4Maybe0;
    VMX_MSR_CR4 Cr4Maybe1;
    VMX_EPT_VPID_CAP EptVpidCapabilities;
    VMX_VMFUNC_CTL VmFuncControls;
} VMX_CAPABILITIES;

//
//  Maybe1 => 1 for each bit that may be 1.
//  Maybe1 => 0 for each bit that may be 0.
//
typedef struct _VMX_CONSTRAINS {
    VMX_PIN_EXECUTION_CTLS PinBasedControlsMaybe1;
    VMX_PIN_EXECUTION_CTLS PinBasedControlsMaybe0;
    VMX_PROC_PRIMARY_CTLS ProcessorControlsMaybe1;
    VMX_PROC_PRIMARY_CTLS ProcessorControlsMaybe0;
    VMX_PROC_SECONDARY_CTLS ProcessorControls2Maybe1;
    VMX_PROC_SECONDARY_CTLS ProcessorControls2Maybe0;
    VMX_EXIT_CTLS ExitControlMaybe1;
    VMX_EXIT_CTLS ExitControlMaybe0;
    VMX_ENTRY_CTLS EntryControlsMaybe1;
    VMX_ENTRY_CTLS EntryControlsMaybe0;
    VMX_MSR_CR0 Cr0Maybe1;
    VMX_MSR_CR0 Cr0Maybe0;
    VMX_MSR_CR4 Cr4Maybe1;
    VMX_MSR_CR4 Cr4Maybe0;
    UINT32 NumberOfCr3TargetValues;
    UINT32 MaxMsrListsSizeInBytes;
    UINT32 VmxTimerLength;
    UINT32 VmcsRevision;
    UINT32 MsegRevisionId;
    BOOLEAN VmEntryInHaltStateSupported;
    BOOLEAN VmEntryInShutdownStateSupported;
    BOOLEAN VmEntryInWaitForSipiStateSupported;
    BOOLEAN ProcessorBasedExecCtrl2Supported;
    BOOLEAN EptSupported;
    BOOLEAN UnrestrictedGuestSupported;
    BOOLEAN VpidSupported;
    BOOLEAN VmfuncSupported;
    BOOLEAN EptpSwitchingSupported;
    BOOLEAN VeSupported;
    VMX_EPT_VPID_CAP EptVpidCapabilities;
} VMX_CONSTRAINS;

//
//  Fixed1 => 1 for each fixed 1 bit.
//  Fixed0 => 0 for each fixed 0 bit.
//
typedef struct VMX_VMCS_FIXED {
    VMX_PIN_EXECUTION_CTLS PinControlFixed1;
    VMX_PIN_EXECUTION_CTLS PinControlFixed0;
    VMX_PROC_PRIMARY_CTLS ProcessorControlsFixed1;
    VMX_PROC_PRIMARY_CTLS ProcessorControlsFixed0;
    VMX_PROC_SECONDARY_CTLS ProcessorControls2Fixed1;
    VMX_PROC_SECONDARY_CTLS ProcessorControls2Fixed0;
    VMX_EXIT_CTLS ExitControlFixed1;
    VMX_EXIT_CTLS ExitControlFixed0;
    VMX_ENTRY_CTLS EntryControlFixed1;
    VMX_ENTRY_CTLS EntryControlFixed0;
    VMX_MSR_CR0 Cr0Fixed1;
    VMX_MSR_CR0 Cr0Fixed0;
    VMX_MSR_CR4 Cr4Fixed1;
    VMX_MSR_CR4 Cr4Fixed0;
} VMX_VMCS_FIXED;

typedef struct _VMX_STATE
{
    VMX_CAPABILITIES Capabilities;
    VMX_CONSTRAINS Constraints;
    VMX_VMCS_FIXED Fixed;
} VMX_STATE, *PVMX_STATE;

extern VMX_STATE gVmState;

VOID
VmInitializeVmState(
    _Inout_ PVMX_STATE VmState
    );

PVOID
VmcsAllocateRegion(
    _In_ UINT32 Revision,
    _In_ UINT32 RegionSize
    );

UINTN
VmMakeCompliantCr0(
    _In_ UINTN Value
    );

UINTN
VmMakeCompliantCr4(
    _In_ UINTN Value
    );

UINTN
VmGetGuestVisibleCr4(
    _In_ UINTN Cr4Value
    );

#endif