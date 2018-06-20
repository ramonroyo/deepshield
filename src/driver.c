#include <ntifs.h>
#include <wdmsec.h>
#include "dsdef.h"
#include "tests.h"

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

_Dispatch_type_(IRP_MJ_SHUTDOWN)
DRIVER_DISPATCH DriverShutdown;

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
DsTimerDPC(
    _In_ struct _KDPC *Dpc,
    _In_opt_ PVOID DeferredContext,
    _In_opt_ PVOID SystemArgument1,
    _In_opt_ PVOID SystemArgument2
    );

BOOLEAN
DsVerifyBuildNumber(
    _In_ ULONG BuildNumber
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, DsAllocateUnicodeString)
#pragma alloc_text(PAGE, DsFreeUnicodeString)
#pragma alloc_text(PAGE, DsCloneUnicodeString)
#if (NTDDI_VERSION >= NTDDI_VISTA) && defined(_WIN64)
#pragma alloc_text(PAGE, DsCheckHvciCompliance)
#endif
#pragma alloc_text(PAGE, DsVerifyBuildNumber)
#endif

#ifdef DBG
DECLARE_CONST_UNICODE_STRING(
    SDDL_DEVOBJ_SYS_ALL_ADM_RW,
    L"D:P(A;;GA;;;SY)(A;;GRGW;;;BA)"
    );
#endif

BOOLEAN gSecuredPageTables;
ULONG gStateFlags;
EX_RUNDOWN_REF gChannelRundown;
PDS_CHANNEL gChannel;
UNICODE_STRING gDriverKeyName;
static BOOLEAN gShutdownCalled;

KDPC ExposeDpc;
PMM_MAP_IO_SPACE_EX DsMmMapIoSpaceEx;
KTIMER ExposeTimer;
PCHAR LeakBuffer;
extern PCHAR LeakData;

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
    ULONG LoadMode = 0;
    BOOLEAN ShutdownCallback = FALSE;
    BOOLEAN SymbolicLink = FALSE;
    BOOLEAN MailboxInitialized = FALSE;

    gShutdownCalled = FALSE;
    gStateFlags = 0;

    WPP_INIT_TRACING( DriverObject, RegistryPath );

    RtlInitUnicodeString( &gDriverKeyName, NULL );

#if (NTDDI_VERSION >= NTDDI_VISTA)
    #pragma warning(disable:4055)
    ExInitializeDriverRuntime( DrvRtPoolNxOptIn );

    if (RtlIsNtDdiVersionAvailable( NTDDI_WIN10 )) {

        RtlInitUnicodeString( &FunctionName, L"MmMapIoSpaceEx" );
        DsMmMapIoSpaceEx = (PMM_MAP_IO_SPACE_EX)
                      MmGetSystemRoutineAddress( &FunctionName );
        
        //
        //  Somehow the page tables address space for RS4 is restricted so it
        //  cannot be mapped freely even for kernel mode.
        //
        gSecuredPageTables = DsVerifyBuildNumber( DS_WINVER_10_RS4 );

    } else {
        DsMmMapIoSpaceEx = NULL;
    }
#endif

    Status = DsInitializeNonPagedPoolList( 256 * PAGE_SIZE );

    if (!NT_SUCCESS( Status )) {
        TraceEvents( TRACE_LEVEL_ERROR, TRACE_INIT,
                     "DsInitializeNonPagedPoolList failed with status %!STATUS!\n",
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
    DriverObject->MajorFunction[IRP_MJ_SHUTDOWN] = DriverShutdown;
    DriverObject->DriverUnload = DriverUnload;

    Status = IoRegisterShutdownNotification( DriverObject->DeviceObject );

    if (!NT_SUCCESS( Status )) {
        goto RoutineExit;
    }

    ShutdownCallback = TRUE;

    Status = IoCreateSymbolicLink( (PUNICODE_STRING) &DsDosDeviceName, 
                                   (PUNICODE_STRING) &DsDeviceName );

    if (!NT_SUCCESS( Status )) {
        goto RoutineExit;
    }

    SymbolicLink = TRUE;
    
    //
    //  The shield must be initialized under System context.
    //
    Status = DsInitializeShield();

    if (!NT_SUCCESS( Status )) {
        //
        //  TODO: log reason and continue.
        //
        Status = STATUS_SUCCESS;
        goto RoutineExit;
    }

    SetFlag( gStateFlags, DSH_GFL_SHIELD_INITIALIZED );

    KeInitializeDpc( &ExposeDpc, DsTimerDPC, NULL );
    KeSetTargetProcessorDpc( &ExposeDpc, 0 );
    KeInitializeTimer( &ExposeTimer );

    Status = DsQueryLoadModePolicy( &LoadMode );

    if (NT_SUCCESS( Status ) && LoadMode == DSH_RUN_MODE_AUTO_START) {

#if (NTDDI_VERSION >= NTDDI_VISTA) && defined(_WIN64)
        if (FALSE == DsCheckHvciCompliance()) {
            //
            //  TODO: log reason and continue.
            //
            Status = STATUS_SUCCESS;
            goto RoutineExit;
        }
#endif

        Status = DsStartShield();

        if (!NT_SUCCESS( Status )) {
            //
            //  TODO: log reason and continue.
            //
            Status = STATUS_SUCCESS;
            goto RoutineExit;
        }

        SetFlag( gStateFlags, DSH_GFL_SHIELD_STARTED );
    }

RoutineExit:

    if (!NT_SUCCESS( Status )) {

        KeCancelTimer( &ExposeTimer );

        if (FlagOn( gStateFlags, DSH_GFL_SHIELD_INITIALIZED )) {

            ClearFlag( gStateFlags, DSH_GFL_SHIELD_INITIALIZED );
            DsFinalizeShield();
        }

        if (SymbolicLink) {
            IoDeleteSymbolicLink( (PUNICODE_STRING) &DsDosDeviceName );
        }

        if (ShutdownCallback) {
            IoUnregisterShutdownNotification( DriverObject->DeviceObject );
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
    IoUnregisterShutdownNotification( DriverObject->DeviceObject );
    KeCancelTimer( &ExposeTimer );

    if (LeakBuffer) {
        ExFreePoolWithTag( LeakBuffer, 'dMsD');
    }

    if (FALSE == gShutdownCalled) {

        if (DsIsShieldRunning()) {
            NT_ASSERT( FlagOn( gStateFlags, DSH_GFL_SHIELD_STARTED ));

            ClearFlag( gStateFlags, DSH_GFL_SHIELD_STARTED );
            DsStopShield();
        }

        if (FlagOn( gStateFlags, DSH_GFL_SHIELD_INITIALIZED )) {

            ClearFlag( gStateFlags, DSH_GFL_SHIELD_INITIALIZED );
            DsFinalizeShield();
        }
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
DriverShutdown(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp
    )
{
    UNREFERENCED_PARAMETER( DeviceObject );

    gShutdownCalled = TRUE;
    
    if (DsIsShieldRunning()) {
        NT_ASSERT( FlagOn( gStateFlags, DSH_GFL_SHIELD_STARTED ));

        ClearFlag( gStateFlags, DSH_GFL_SHIELD_STARTED );
        DsStopShield();
    }

    if (FlagOn( gStateFlags, DSH_GFL_SHIELD_INITIALIZED )) {

        ClearFlag( gStateFlags, DSH_GFL_SHIELD_INITIALIZED );
        DsFinalizeShield();
    }

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    return STATUS_SUCCESS;
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

        case IOCTL_MELTDOWN_EXPOSE:
        {
            Status = DsCtlMeltdownExpose( Irp, IrpStack );
            break;
        }

//
// TODO: Enable only-debug when tests are finished.
//
// #ifdef DEBUG
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

#if (NTDDI_VERSION >= NTDDI_VISTA) && defined(_WIN64)
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

#define _DS_MM_HINT_T0  1

VOID 
DsTimerDPC(
    _In_ struct _KDPC *Dpc,
    _In_opt_ PVOID DeferredContext,
    _In_opt_ PVOID SystemArgument1,
    _In_opt_ PVOID SystemArgument2
    )
{
    INT Idx = 0;

    UNREFERENCED_PARAMETER( Dpc );
    UNREFERENCED_PARAMETER( DeferredContext );
    UNREFERENCED_PARAMETER( SystemArgument1 );
    UNREFERENCED_PARAMETER( SystemArgument2 );

    for (; Idx < 64; Idx += 32) {
        _mm_prefetch( LeakBuffer + Idx, _DS_MM_HINT_T0 );
    }
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

BOOLEAN
DsVerifyBuildNumber(
    _In_ ULONG BuildNumber
    )
/*++
Routine Description:

    This routine verify the presence of an OS version that matches or it is
    higher than a given build number.

Arguments:
    
    BuildNumber - input build number to verify.
    
Return value:

    TRUE if result is positive, FALSE otherwise.

--*/
{

    NTSTATUS Status;
    RTL_OSVERSIONINFOEXW VersionInfo = {0};
    ULONGLONG ConditionMask = 0;

    PAGED_CODE();
   
    VersionInfo.dwOSVersionInfoSize = sizeof(VersionInfo);
    VersionInfo.dwBuildNumber = BuildNumber;

    VER_SET_CONDITION( ConditionMask, VER_BUILDNUMBER, VER_GREATER_EQUAL );

    Status = RtlVerifyVersionInfo( &VersionInfo, 
                                   VER_BUILDNUMBER,
                                   ConditionMask );

    if (NT_SUCCESS ( Status )) {
        return TRUE;
    }

    return FALSE;
}