#include <ntifs.h>
#include <wdmsec.h>
#include "dsdef.h"

#if defined(WPP_EVENT_TRACING)
#include "driver.tmh"
#else
#define _DRIVER_NAME_ "DeepShield"
ULONG DebugLevel = TRACE_LEVEL_INFORMATION;
ULONG DebugFlag = 0xff;
#endif

DECLARE_CONST_UNICODE_STRING( DsDeviceName, DS_WINNT_DEVICE_NAME );
DECLARE_CONST_UNICODE_STRING( DsDosDeviceName, DS_MSDOS_DEVICE_NAME );

DRIVER_INITIALIZE DriverEntry;
DRIVER_UNLOAD DriverUnload;

_Dispatch_type_(IRP_MJ_DEVICE_CONTROL)
DRIVER_DISPATCH DriverDeviceControl;

_Dispatch_type_(IRP_MJ_CREATE)
_Dispatch_type_(IRP_MJ_CLOSE)
DRIVER_DISPATCH DriverDeviceCreateClose;

_When_(return==0, _Post_satisfies_(String->Buffer != NULL))
NTSTATUS
DsAllocateUnicodeString(
    _Inout_ PUNICODE_STRING String
    );

VOID
DsFreeUnicodeString(
    _Inout_ PUNICODE_STRING String
    );

NTSTATUS
DsCloneUnicodeString(
    _Inout_ PUNICODE_STRING DestinationString,
    _In_ PCUNICODE_STRING SourceString
    );

#if (NTDDI_VERSION >= NTDDI_VISTA) && defined(_WIN64)
BOOLEAN
DsCheckHvciCompliance(
    VOID
    );
#endif

VOID
DsInitializeVmxFeature(
    _Inout_ PDS_VMX_FEATURE VmxFeature
);

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, DsAllocateUnicodeString)
#pragma alloc_text(PAGE, DsFreeUnicodeString)
#pragma alloc_text(PAGE, DsCloneUnicodeString)
#if (NTDDI_VERSION >= NTDDI_VISTA) && defined(_WIN64)
#pragma alloc_text(PAGE, DsCheckHvciCompliance)
#endif
#pragma alloc_text(PAGE, DsInitializeVmxFeature)
#endif

#ifdef DBG
DECLARE_CONST_UNICODE_STRING(
    SDDL_DEVOBJ_SYS_ALL_ADM_RW,
    L"D:P(A;;GA;;;SY)(A;;GRGW;;;BA)"
    );
#endif

PVOID gPowerRegistration;
BOOLEAN gSecuredPageTables;
BOOLEAN gEncodedDebuggerDataBlock;
UINTN gSystemPageDirectoryTable;
ULONG gStateFlags;
EX_RUNDOWN_REF gChannelRundown;
PDS_CHANNEL gChannel;
UNICODE_STRING gDriverKeyName;
PMM_MAP_IO_SPACE_EX DsMmMapIoSpaceEx;
DS_VMX_FEATURE gVmxFeature;
VMX_STATE gVmState;

NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    PDEVICE_OBJECT DeviceObject = NULL;
    PCUNICODE_STRING DeviceSecurityString;
    UNICODE_STRING FunctionName;
    BOOLEAN MailboxInitialized = FALSE;
    BOOLEAN SymbolicLink = FALSE;

    gSecuredPageTables = FALSE;
    gEncodedDebuggerDataBlock = FALSE;
    gStateFlags = 0;

    WPP_INIT_TRACING( DriverObject, RegistryPath );

    RtlInitUnicodeString( &gDriverKeyName, NULL );
    gSystemPageDirectoryTable = __readcr3();

#if (NTDDI_VERSION >= NTDDI_VISTA)

#pragma warning(disable:4055)
    ExInitializeDriverRuntime( DrvRtPoolNxOptIn );

    if (RtlIsNtDdiVersionAvailable( NTDDI_WIN10 )) {

        RtlInitUnicodeString( &FunctionName, L"MmMapIoSpaceEx" );
        DsMmMapIoSpaceEx = (PMM_MAP_IO_SPACE_EX)
                      MmGetSystemRoutineAddress( &FunctionName );
#ifdef _WIN64
        //
        //  Somehow the page tables address space for RS4 is restricted so it
        //  cannot be mapped freely even for kernel mode.
        //
        gSecuredPageTables = OsVerifyBuildNumber( DS_WINVER_10_RS4 );
        gEncodedDebuggerDataBlock = OsVerifyBuildNumber( DS_WINVER_10_RS5 );
#endif

    } else {
        DsMmMapIoSpaceEx = NULL;
    }
#endif

    DsInitializeVmxFeature( &gVmxFeature );
    RtlCopyMemory( &gVmState, &gVmxFeature.VmState, sizeof( VMX_STATE ) );

    Status = DsInitializeNonPagedPoolList( 256 * PAGE_SIZE );

    if (!NT_SUCCESS( Status )) {
        TraceEvents( TRACE_LEVEL_ERROR, TRACE_INIT,
                     "DsInitializeNonPagedPoolList failed with Status %!STATUS!\n",
                     Status );
        return Status;
    }

    Status = RtlMailboxInitialize( &gSecureMailbox, MAILBOX_POOL_DEFAULT_SIZE );

    if ( !NT_SUCCESS( Status ) ) {
        goto RoutineExit;
    }

    MailboxInitialized = TRUE;

    Status = DsCloneUnicodeString( &gDriverKeyName, RegistryPath );

    if (!NT_SUCCESS( Status )) {
        goto RoutineExit;
    }

#ifdef DBG
    DeviceSecurityString = &SDDL_DEVOBJ_SYS_ALL_ADM_RWX_WORLD_RWX_RES_RWX;
#else
    DeviceSecurityString = &SDDL_DEVOBJ_SYS_ALL;
#endif

    //
    //  Kernel code and user mode code running as *SYSTEM* is allowed to open
    //  the device for any access. This tight security settings and let its
    //  service to open the device up.
    //
    Status = IoCreateDeviceSecure( DriverObject,
                                   0,
                                   (PUNICODE_STRING) &DsDeviceName,
                                   FILE_DEVICE_UNKNOWN,
                                   FILE_DEVICE_SECURE_OPEN,
                                   FALSE,
                                   DeviceSecurityString,
                                   NULL,
                                   &DeviceObject );

    if (!NT_SUCCESS( Status )) {
        goto RoutineExit;
    }

    DriverObject->MajorFunction[IRP_MJ_CREATE] = DriverDeviceCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = DriverDeviceCreateClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DriverDeviceControl;
    DriverObject->DriverUnload = DriverUnload;

    Status = IoCreateSymbolicLink( (PUNICODE_STRING) &DsDosDeviceName, 
                                   (PUNICODE_STRING) &DsDeviceName );

    if (!NT_SUCCESS( Status )) {
        goto RoutineExit;
    }

    SymbolicLink = TRUE;

    //
    //  Register the power change callback to load / unload VMM on
    //  sleep (S1/S2/S3) and hibernate (S4) state changes.
    //

    gPowerRegistration = 
        DsRegisterPowerChangeCallback( DsPowerChangeCallback, NULL, 0 );

    if (gPowerRegistration) {
        SetFlag( gStateFlags, DSH_GFL_POWER_REGISTERED );
    }

    Status = DsInitializeShield();

    if (!NT_SUCCESS( Status )) {
        Status = STATUS_SUCCESS;
        goto RoutineExit;
    }
    
    SetFlag( gStateFlags, DSH_GFL_SHIELD_INITIALIZED );

RoutineExit:

    if (!NT_SUCCESS( Status )) {

        if (FlagOn( gStateFlags, DSH_GFL_SHIELD_INITIALIZED )) {

            ClearFlag( gStateFlags, DSH_GFL_SHIELD_INITIALIZED );
            DsFinalizeShield();
        }

        if (FlagOn( gStateFlags, DSH_GFL_POWER_REGISTERED )) {

            ClearFlag( gStateFlags, DSH_GFL_POWER_REGISTERED );
            DsDeregisterPowerChangeCallback( gPowerRegistration );
        }

        if (SymbolicLink) {
            IoDeleteSymbolicLink( (PUNICODE_STRING) &DsDosDeviceName );
        }

        if (DeviceObject) {
            IoDeleteDevice( DriverObject->DeviceObject );
        }

        if (gDriverKeyName.Length) {
            DsFreeUnicodeString( &gDriverKeyName );
        }

        if (MailboxInitialized) {
            RtlMailboxDestroy( &gSecureMailbox );
        }

        DsDeleteNonPagedPoolList();
        WPP_CLEANUP( DriverObject );
    }

    return Status;
}

VOID
DriverUnload(
    _In_ PDRIVER_OBJECT DriverObject
    )
 {
    if (DsIsShieldRunning()) {
        NT_ASSERT( FlagOn( gStateFlags, DSH_GFL_SHIELD_STARTED ));

        ClearFlag( gStateFlags, DSH_GFL_SHIELD_STARTED );
        DsStopShield();
    }

    if (FlagOn( gStateFlags, DSH_GFL_SHIELD_INITIALIZED )) {

        ClearFlag( gStateFlags, DSH_GFL_SHIELD_INITIALIZED );
        DsFinalizeShield();
    }

    if (FlagOn( gStateFlags, DSH_GFL_POWER_REGISTERED )) {

        ClearFlag( gStateFlags, DSH_GFL_POWER_REGISTERED );
        DsDeregisterPowerChangeCallback( gPowerRegistration );
    }

    if (FlagOn( gStateFlags, DSH_GFL_CHANNEL_SETUP )) {
        ClearFlag( gStateFlags, DSH_GFL_CHANNEL_SETUP );

        ExWaitForRundownProtectionRelease( &gChannelRundown );
        ExRundownCompleted( &gChannelRundown );

        DsDestroyChannel( gChannel, UnloadDriverReason );
    }

    IoDeleteSymbolicLink( (PUNICODE_STRING) &DsDosDeviceName );
    IoDeleteDevice( DriverObject->DeviceObject );

    DsFreeUnicodeString( &gDriverKeyName );
    RtlMailboxDestroy( &gSecureMailbox);

    DsDeleteNonPagedPoolList();
    WPP_CLEANUP( DriverObject );
}

NTSTATUS
DriverDeviceCreateClose(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp
    )
/*++

Routine Description:

   Dispatch routine to handle create / close IRPs.  No action is performed
   other than completing the request successfully.

Arguments:

    DeviceObject - a pointer to the object that represents the device
    that I/O is to be done on.

    Irp - pointer to an I/O Request Packet.

Return Value:

   NT status code

--*/
{
    UNREFERENCED_PARAMETER( DeviceObject );

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    return STATUS_SUCCESS;
}

NTSTATUS
DriverDeviceControl(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION IrpStack;

    UNREFERENCED_PARAMETER( DeviceObject );

    IrpStack = IoGetCurrentIrpStackLocation( Irp );

    Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
    Irp->IoStatus.Information = 0;

    switch (IrpStack->Parameters.DeviceIoControl.IoControlCode)
    {
        case IOCTL_SHIELD_STATUS:
        {
            Status = DsCltGetShieldState( Irp, IrpStack );
            break;
        }
        case IOCTL_SHIELD_CONTROL:
        {
            Status = DsCtlShieldControl( Irp, IrpStack );
            break;
        }

        case IOCTL_SHIELD_CHANNEL_SETUP:
        {
            Status = DsCtlShieldChannelSetup( Irp, IrpStack );
            break;
        }

        case IOCTL_SHIELD_CHANNEL_TEARDOWN:
        {
            Status = DsCtlShieldChannelTeardown( Irp, IrpStack );
            break;
        }

        case IOCTL_SHIELD_GET_VMFEATURE:
        {
            Status = DsCtlShieldGetVmFeature( Irp, IrpStack );
            break;
        }

//
// TODO: Enable only-debug when tests are finished.
// #ifdef DBG
//
        case IOCTL_TEST_RDTSC:
        {
            Status = DsCtlTestRdtscDetection( Irp, IrpStack );
            break;
        }
// #endif
    }

    if (STATUS_PENDING != Status) {
        Irp->IoStatus.Status = Status;
        IoCompleteRequest( Irp, IO_NO_INCREMENT );
    }

    return Status;
}

_When_(return==0, _Post_satisfies_(String->Buffer != NULL))
NTSTATUS
DsAllocateUnicodeString (
    _Inout_ PUNICODE_STRING String
    )
{
    PAGED_CODE();

    String->Buffer = ExAllocatePoolWithTag( NonPagedPool,
                                            String->MaximumLength,
                                            'sUsD' );

    if (String->Buffer == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    String->Length = 0;

    return STATUS_SUCCESS;
}

VOID
DsFreeUnicodeString(
    _Inout_ PUNICODE_STRING String
    )
{
    PAGED_CODE();

    if (String->Buffer) {
        ExFreePoolWithTag( String->Buffer, 'sUsD' );
        String->Buffer = NULL;
    }

    String->Length = String->MaximumLength = 0;
    String->Buffer = NULL;
}

NTSTATUS
DsCloneUnicodeString(
    _Inout_ PUNICODE_STRING DestinationString,
    _In_ PCUNICODE_STRING SourceString
    )
{
    NTSTATUS Status;

    PAGED_CODE();

    RtlInitUnicodeString( DestinationString, NULL );
    DestinationString->MaximumLength = SourceString->Length;

    Status = DsAllocateUnicodeString( DestinationString );

    if (NT_SUCCESS( Status )) {
        RtlCopyUnicodeString( DestinationString, SourceString );
    }

    return Status;
}


BOOLEAN
DsCheckCpuFamilyIntel(
    VOID
    )
{
    CPU_INFO CpuInfo = { 0 };

    //
    //  Just run for GenuineIntel.
    //
    __cpuid( &CpuInfo, CPUID_BASIC_INFORMATION );

    return CPUID_VALUE_ECX( CpuInfo ) == 'letn' &&
           CPUID_VALUE_EDX( CpuInfo ) == 'Ieni' &&
           CPUID_VALUE_EBX( CpuInfo ) == 'uneG';
}

BOOLEAN
DsCheckCpuVmxCapable(
    VOID
    )
{
    CPU_INFO CpuInfo = { 0 };

    //
    //  Check whether hypervisor support is present.
    //
    __cpuid( &CpuInfo, CPUID_FEATURE_INFORMATION );

    if (0 == (CPUID_VALUE_ECX( CpuInfo ) & CPUID_LEAF_1H_ECX_VMX)) {
       return FALSE;
    }

    return TRUE;
}

BOOLEAN
DsCheckVmxFirmwareState(
    VOID
    )
{
    UINT64 RequiredFeature = IA32_FC_LOCK | IA32_FC_ENABLE_VMXON_OUTSMX;

    //
    //  Strictly we need IA32_FC_ENABLE_VMXON_OUTSMX. If the
    //  IA32_FC_LOCK bit isn't set, we're free to write to the MSR.
    //  System BIOS uses this bit to provide a setup option for BIOS to disable
    //  support for VMX.
    //

    if ((__readmsr( IA32_FEATURE_CONTROL ) & RequiredFeature)
                                          != RequiredFeature) {
        return FALSE;
    }

    return TRUE;
}

#if (NTDDI_VERSION >= NTDDI_VISTA) && defined(_WIN64)
BOOLEAN
DsCheckMicrosoftHyperV(
    VOID
    )
{
    CPU_INFO CpuInfo = { 0 };

    //
    //  Check if the hypervisor is Hyper-V.
    //
    __cpuid( &CpuInfo, CPUID_VMM_VENDOR_SIGNATURE );

   if (CPUID_VALUE_EBX( CpuInfo ) == (UINT32)'Micr' &&
       CPUID_VALUE_ECX( CpuInfo ) == (UINT32)'osof' &&
       CPUID_VALUE_EDX( CpuInfo ) == (UINT32)'t Hv' &&
       CPUID_VALUE_EAX( CpuInfo ) >= 0x40000005) {

       __cpuid( &CpuInfo, CPUID_VMM_INTERFACE_SIGNATURE );

        if (CPUID_VALUE_EAX( CpuInfo ) == (UINT32)'1#vH') {
            //
            //  The check fails when Hyper-V is detected.
            //
            return FALSE;
        }
    }

    return TRUE;
}

BOOLEAN
DsCheckHvciCompliance(
    VOID
    )
{
    NTSTATUS Status;
    SYSTEM_CODEINTEGRITY_INFORMATION CodeIntegrityInfo;

    PAGED_CODE();

    if (!RtlIsNtDdiVersionAvailable( NTDDI_WIN10 )) {
        return TRUE;
    }

    CodeIntegrityInfo.Length = sizeof( SYSTEM_CODEINTEGRITY_INFORMATION );
    Status = ZwQuerySystemInformation( SystemCodeIntegrityInformation,
                                       &CodeIntegrityInfo, 
                                       sizeof( CodeIntegrityInfo ),
                                       NULL );

    if (!NT_SUCCESS( Status )) {
        //
        //  TODO: log and fall back to the true case.
        //
        return TRUE;
    }

    if (CodeIntegrityInfo.CodeIntegrityOptions 
        &   ( CODEINTEGRITY_OPTION_HVCI_KMCI_ENABLED
            | CODEINTEGRITY_OPTION_HVCI_KMCI_AUDITMODE_ENABLED
            | CODEINTEGRITY_OPTION_HVCI_KMCI_STRICTMODE_ENABLED
            | CODEINTEGRITY_OPTION_HVCI_IUM_ENABLED )) {

        return FALSE;
    }

    return TRUE;
}
#endif

VOID
DsInitializeVmxFeature(
    _Inout_ PDS_VMX_FEATURE VmxFeature
    )
{
    PAGED_CODE();

    RtlZeroMemory( VmxFeature, sizeof( DS_VMX_FEATURE ) );

    if (FALSE == DsCheckCpuFamilyIntel() ) {
        VmxFeature->Bits.NoIntelCpu = 1;
        return;
    }

#if (NTDDI_VERSION >= NTDDI_VISTA) && defined(_WIN64)

    if (FALSE == DsCheckHvciCompliance() 
        || FALSE == DsCheckMicrosoftHyperV()) {
        //
        //  Nothing else to check. In this situation a single RDMSR might
        //  cause Hyper-V to raise a privileged instruction exception.
        //

        VmxFeature->Bits.HvciEnabled = 1;
        return;
    }

#endif

    if (FALSE == DsCheckCpuVmxCapable() ) {
        VmxFeature->Bits.NoVtxExtension = 1;
        return;
    } 

    if (FALSE == DsCheckVmxFirmwareState() ) {
        VmxFeature->Bits.FirmwareDisabled = 1;
        return;
    }

    VmInitializeVmState( &VmxFeature->VmState );
}

#if !defined(WPP_EVENT_TRACING)
VOID
TraceEvents(
    _In_ ULONG DebugPrintLevel,
    _In_ ULONG DebugPrintFlag,
    __drv_formatString( printf ) __in PCSTR DebugMessage,
    ...
    )
/*++
Routine Description:
    
    Debug print for the DeepShield driver.

Arguments:
    
    TraceEventsLevel - print level between 0 and 3, with 3 the most verbose

Return Value:
    
    None.

 --*/
 {
#if DBG
#define     TEMP_BUFFER_SIZE        1024
    va_list    list;
    CHAR       debugMessageBuffer[TEMP_BUFFER_SIZE];
    NTSTATUS   Status;

    va_start( list, DebugMessage );

    if (DebugMessage) {
        //
        //  Using new safe string functions instead of _vsnprintf.
        //
        Status = RtlStringCbVPrintfA( debugMessageBuffer,
                                      sizeof(debugMessageBuffer),
                                      DebugMessage,
                                      list );
        if (!NT_SUCCESS( Status )) {
            DbgPrint (_DRIVER_NAME_": RtlStringCbVPrintfA failed 0x%x\n", Status);
            return;
        }

        if (DebugPrintLevel <= TRACE_LEVEL_ERROR 
            || (DebugPrintLevel <= DebugLevel &&
              ((DebugPrintFlag & DebugFlag) == DebugPrintFlag))) {

            DbgPrint( "%s %s", _DRIVER_NAME_, debugMessageBuffer );
        }
    }

    va_end( list );
    return;
#else
    UNREFERENCED_PARAMETER(DebugPrintLevel);
    UNREFERENCED_PARAMETER(DebugPrintFlag);
    UNREFERENCED_PARAMETER(DebugMessage);
#endif
}
#endif

/*
VOID
GetPhysicalAddressWidth(
    _Inout_ PUINT8 PhysicalAddressBits
    )
{
    UINT32 RegEax;

    AsmCpuid(CPUID_EXTENDED_INFORMATION, &RegEax, NULL, NULL, NULL);

    if (RegEax >= CPUID_EXTENDED_ADDRESS_SIZE) {
        AsmCpuid( CPUID_EXTENDED_ADDRESS_SIZE, &RegEax, NULL, NULL, NULL );

        *PhysicalAddressBits = (UINT8)RegEax;
    }
    else {
        *PhysicalAddressBits = 36;
    }
}
*/