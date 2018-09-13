#ifndef __DSDEF_H
#define __DSDEF_H

#include <ntifs.h>
#include <ntstrsafe.h>
#include <stdarg.h>
#include <ioctl.h>
#include "wdk7.h"
#include "wpp.h"
#include "ringbuf.h"
#include "mailbox.h"
#include "channel.h"
#include "power.h"
#include "shield.h"
#include "policy.h"
#include "ioctl.h"
#include "tests.h"
#include "mem.h"
#include "smp.h"
#include "os.h"
#include "vmx.h"
#include "vmcsinit.h"

#pragma warning(disable:4201)   // nameless struct/union

extern BOOLEAN gSecuredPageTables;
extern ULONG gStateFlags;
extern EX_RUNDOWN_REF gChannelRundown;
extern PDS_CHANNEL gChannel;
extern UNICODE_STRING gDriverKeyName;
extern MAILBOX gSecureMailbox;

#define DSH_POLICY_KEY_NAME    L"Parameters"
#define DSH_RUN_MODE_POLICY    L"OperationMode"

#define DS_WINVER_10_RS4       (17134UL)

//
//  Preserve the state of the protection between driver instances.
//
#define DSH_RUN_MODE_AUTO_START  (40)
#define DSH_RUN_MODE_DISABLED    (0)

//
//  Global flags to control driver runtime state.
//
#define DSH_GFL_SHIELD_INITIALIZED  0x00000001
#define DSH_GFL_SHIELD_STARTED      0x00000002
#define DSH_GFL_SHIELD_SUSPENDED    0x00000004
#define DSH_GFL_CHANNEL_SETUP       0x00000008
#define DSH_GFL_POWER_REGISTERED    0x00000010

#define DSH_VMX_ABSENT(v)    \
    ((BOOLEAN)(((PDS_VMX_STATE)(v))->Flags.AllFlags != 0))

//
//  These macros are used to test, set and clear flags respectivly
//

#ifndef FlagOn
#define FlagOn(_F,_SF)        ((_F) & (_SF))
#endif

#ifndef BooleanFlagOn
#define BooleanFlagOn(F,SF)   ((BOOLEAN)(((F) & (SF)) != 0))
#endif

#ifndef SetFlag
#define SetFlag(_F,_SF)       ((_F) |= (_SF))
#endif

#ifndef ClearFlag
#define ClearFlag(_F,_SF)     ((_F) &= ~(_SF))
#endif


#define DsInitializeNonPagedPoolList(_x_) MemInit(_x_)
#define DsDeleteNonPagedPoolList() MemDone()

#ifdef DSH_PRIVATE_POOL
#define DsAllocatePoolWithTag(_p_, _x_, _t_) MemAllocAligned(_x_, 16)
#define DsFreePoolWithTag(_p_, _t_) MemFree(_p_)
#else
#define DsAllocatePoolWithTag(_p_, _x_, _t_) ExAllocatePoolWithTag (_p_, _x_, _t_)
#define DsFreePoolWithTag(_p_, _t_) ExFreePoolWithTag( _p_, _t_)
#endif

typedef
PVOID
(*PMM_MAP_IO_SPACE_EX) (
    _In_ PHYSICAL_ADDRESS PhysicalAddress,
    _In_ SIZE_T NumberOfBytes,
    _In_ ULONG Protect
    );

extern PMM_MAP_IO_SPACE_EX DsMmMapIoSpaceEx;

typedef struct _DS_VMX_STATE {

    ULONG Length;

    union {
        struct {
            ULONG NoIntelCpu : 1;
            ULONG HvciEnabled : 1;
            ULONG NoVtxExtension : 1;
            ULONG FirmwareDisabled : 1;
            ULONG Spare : 28;
        };

        ULONG AllFlags;
    } Flags;

} DS_VMX_STATE, *PDS_VMX_STATE;

#if (NTDDI_VERSION >= NTDDI_VISTA) && defined(_WIN64)

typedef enum _SYSTEM_INFORMATION_CLASS {
    SystemBasicInformation = 0,
    SystemProcessorInformation = 1,
    SystemPerformanceInformation = 2,
    SystemTimeOfDayInformation = 3,
    SystemPathInformation = 4,
    SystemProcessInformation = 5,
    SystemCallCountInformation = 6,
    SystemDeviceInformation = 7,
    SystemProcessorPerformanceInformation = 8,
    SystemFlagsInformation = 9,
    SystemCallTimeInformation = 10,
    SystemModuleInformation = 11,
    SystemLocksInformation = 12,
    SystemStackTraceInformation = 13,
    SystemPagedPoolInformation = 14,
    SystemNonPagedPoolInformation = 15,
    SystemHandleInformation = 16,
    SystemObjectInformation = 17,
    SystemPageFileInformation = 18,
    SystemVdmInstemulInformation = 19,
    SystemVdmBopInformation = 20,
    SystemFileCacheInformation = 21,
    SystemPoolTagInformation = 22,
    SystemInterruptInformation = 23,
    SystemDpcBehaviorInformation = 24,
    SystemFullMemoryInformation = 25,
    SystemLoadGdiDriverInformation = 26,
    SystemUnloadGdiDriverInformation = 27,
    SystemTimeAdjustmentInformation = 28,
    SystemSummaryMemoryInformation = 29,
    SystemMirrorMemoryInformation = 30,
    SystemPerformanceTraceInformation = 31,
    SystemObsolete0 = 32,
    SystemExceptionInformation = 33,
    SystemCrashDumpStateInformation = 34,
    SystemKernelDebuggerInformation = 35,
    SystemContextSwitchInformation = 36,
    SystemRegistryQuotaInformation = 37,
    SystemExtendedServiceTableInformation = 38,
    SystemPrioritySeparation = 39,
    SystemVerifierAddDriverInformation = 40,
    SystemVerifierRemoveDriverInformation = 41,
    SystemProcessorIdleInformation = 42,
    SystemLegacyDriverInformation = 43,
    SystemCurrentTimeZoneInformation = 44,
    SystemLookasideInformation = 45,
    SystemTimeSlipNotification = 46,
    SystemSessionCreate = 47,
    SystemSessionDetach = 48,
    SystemSessionInformation = 49,
    SystemRangeStartInformation = 50,
    SystemVerifierInformation = 51,
    SystemVerifierThunkExtend = 52,
    SystemSessionProcessInformation = 53,
    SystemLoadGdiDriverInSystemSpace = 54,
    SystemNumaProcessorMap = 55,
    SystemPrefetcherInformation = 56,
    SystemExtendedProcessInformation = 57,
    SystemRecommendedSharedDataAlignment = 58,
    SystemComPlusPackage = 59,
    SystemNumaAvailableMemory = 60,
    SystemProcessorPowerInformation = 61,
    SystemEmulationBasicInformation = 62,
    SystemEmulationProcessorInformation = 63,
    SystemExtendedHandleInformation = 64,
    SystemLostDelayedWriteInformation = 65,
    SystemBigPoolInformation = 66,
    SystemSessionPoolTagInformation = 67,
    SystemSessionMappedViewInformation = 68,
    SystemHotpatchInformation = 69,
    SystemObjectSecurityMode = 70,
    SystemWatchdogTimerHandler = 71,
    SystemWatchdogTimerInformation = 72,
    SystemLogicalProcessorInformation = 73,
    SystemWow64SharedInformationObsolete = 74,
    SystemRegisterFirmwareTableInformationHandler = 75,
    SystemFirmwareTableInformation = 76,
    SystemModuleInformationEx = 77,
    SystemVerifierTriageInformation = 78,
    SystemSuperfetchInformation = 79,
    SystemMemoryListInformation = 80,
    SystemFileCacheInformationEx = 81,
    SystemThreadPriorityClientIdInformation = 82,
    SystemProcessorIdleCycleTimeInformation = 83,
    SystemVerifierCancellationInformation = 84,
    SystemProcessorPowerInformationEx = 85,
    SystemRefTraceInformation = 86,
    SystemSpecialPoolInformation = 87,
    SystemProcessIdInformation = 88,
    SystemErrorPortInformation = 89,
    SystemBootEnvironmentInformation = 90,
    SystemHypervisorInformation = 91,
    SystemVerifierInformationEx = 92,
    SystemTimeZoneInformation = 93,
    SystemImageFileExecutionOptionsInformation = 94,
    SystemCoverageInformation = 95,
    SystemPrefetchPatchInformation = 96,
    SystemVerifierFaultsInformation = 97,
    SystemSystemPartitionInformation = 98,
    SystemSystemDiskInformation = 99,
    SystemProcessorPerformanceDistribution = 100,
    SystemNumaProximityNodeInformation = 101,
    SystemDynamicTimeZoneInformation = 102,
    SystemCodeIntegrityInformation = 103,
    SystemProcessorMicrocodeUpdateInformation = 104,
    SystemProcessorBrandString = 105,
    SystemVirtualAddressInformation = 106,
    SystemLogicalProcessorAndGroupInformation = 107,
    SystemProcessorCycleTimeInformation = 108,
    SystemStoreInformation = 109,
    SystemRegistryAppendString = 110,
    SystemAitSamplingValue = 111,
    SystemVhdBootInformation = 112,
    SystemCpuQuotaInformation = 113,
    SystemNativeBasicInformation = 114,
    SystemErrorPortTimeouts = 115,
    SystemLowPriorityIoInformation = 116,
    SystemBootEntropyInformation = 117,
    SystemVerifierCountersInformation = 118,
    SystemPagedPoolInformationEx = 119,
    SystemSystemPtesInformationEx = 120,
    SystemNodeDistanceInformation = 121,
    SystemAcpiAuditInformation = 122,
    SystemBasicPerformanceInformation = 123,
    SystemQueryPerformanceCounterInformation = 124,
    SystemSessionBigPoolInformation = 125,
    SystemBootGraphicsInformation = 126,
    SystemScrubPhysicalMemoryInformation = 127,
    SystemBadPageInformation = 128,
    SystemProcessorProfileControlArea = 129,
    SystemCombinePhysicalMemoryInformation = 130,
    SystemEntropyInterruptTimingInformation = 131,
    SystemConsoleInformation = 132,
    SystemPlatformBinaryInformation = 133,
    SystemThrottleNotificationInformation = 134,
    SystemPolicyInformation = 134,
    SystemHypervisorProcessorCountInformation = 135,
    SystemDeviceDataInformation = 136,
    SystemDeviceDataEnumerationInformation = 137,
    SystemMemoryTopologyInformation = 138,
    SystemMemoryChannelInformation = 139,
    SystemBootLogoInformation = 140,
    SystemProcessorPerformanceInformationEx = 141,
    SystemSpare0 = 142,
    SystemSecureBootPolicyInformation = 143,
    SystemPageFileInformationEx = 144,
    SystemSecureBootInformation = 145,
    SystemEntropyInterruptTimingRawInformation = 146,
    SystemPortableWorkspaceEfiLauncherInformation = 147,
    SystemFullProcessInformation = 148,
    SystemKernelDebuggerInformationEx = 149,
    SystemBootMetadataInformation = 150,
    SystemSoftRebootInformation = 151,
    SystemElamCertificateInformation = 152,
    SystemOfflineDumpConfigInformation = 153,
    SystemProcessorFeaturesInformation = 154,
    SystemRegistryReconciliationInformation = 155,
    SystemEdidInformation = 156,
    SystemManufacturingInformation = 157,
    SystemEnergyEstimationConfigInformation = 158,
    SystemHypervisorDetailInformation = 159,
    SystemProcessorCycleStatsInformation = 160,
    SystemVmGenerationCountInformation = 161,
    SystemTrustedPlatformModuleInformation = 162,
    SystemKernelDebuggerFlags = 163,
    SystemCodeIntegrityPolicyInformation = 164,
    SystemIsolatedUserModeInformation = 165,
    SystemHardwareSecurityTestInterfaceResultsInformation = 166,
    SystemSingleModuleInformation = 167,
    SystemAllowedCpuSetsInformation = 168,
    SystemDmaProtectionInformation = 169,
    SystemInterruptCpuSetsInformation = 170,
    SystemSecureBootPolicyFullInformation = 171,
    SystemCodeIntegrityPolicyFullInformation = 172,
    SystemAffinitizedInterruptProcessorInformation = 173,
    SystemRootSiloInformation = 174,
    SystemCpuSetInformation = 175,
    SystemCpuSetTagInformation = 176,
    MaxSystemInfoClass = 177,
} SYSTEM_INFORMATION_CLASS;

#define CODEINTEGRITY_OPTION_HVCI_KMCI_ENABLED              0x0400
#define CODEINTEGRITY_OPTION_HVCI_KMCI_AUDITMODE_ENABLED    0x0800
#define CODEINTEGRITY_OPTION_HVCI_KMCI_STRICTMODE_ENABLED   0x1000
#define CODEINTEGRITY_OPTION_HVCI_IUM_ENABLED               0x2000

typedef struct _SYSTEM_CODEINTEGRITY_INFORMATION {
    ULONG  Length;
    ULONG  CodeIntegrityOptions;
} SYSTEM_CODEINTEGRITY_INFORMATION, *PSYSTEM_CODEINTEGRITY_INFORMATION;

extern NTSYSAPI
NTSTATUS
NTAPI
ZwQuerySystemInformation(
    _In_ SYSTEM_INFORMATION_CLASS SystemInformationClass,
    _Out_writes_bytes_opt_(SystemInformationLength) PVOID SystemInformation,
    _In_ ULONG SystemInformationLength,
    _Out_opt_ PULONG ReturnLength
);
#endif // (NTDDI_VERSION >= NTDDI_VISTA) && defined(_WIN64)


#define TIMEOUT_TO_SEC              ((LONGLONG) 1 * 10 * 1000 * 1000)
#define TIMEOUT_TO_MS               ((LONGLONG) 1 * 10 * 1000)
#define TIMEOUT_TO_US               ((LONGLONG) 1 * 10)

LONGLONG
FORCEINLINE
REL_TIMEOUT_IN_MS(
    _In_ ULONGLONG Time
    )
{
    return Time * -1 * TIMEOUT_TO_MS;
}

LONGLONG
FORCEINLINE
ABS_TIMEOUT_IN_MS(
    _In_ ULONGLONG Time
    )
{
    return Time * 1 * TIMEOUT_TO_MS;
}

#endif