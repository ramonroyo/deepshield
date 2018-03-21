#include <ntddk.h>
#include <wdmsec.h>
#include "dsdef.h"

DECLARE_CONST_UNICODE_STRING( DsDeviceName, DS_WINNT_DEVICE_NAME );
DECLARE_CONST_UNICODE_STRING( DsDosDeviceName, DS_MSDOS_DEVICE_NAME );

static UNICODE_STRING gDriverKeyName;
static ULONG gStateFlags;
static BOOLEAN gShutdownCalled;

DRIVER_INITIALIZE DriverEntry;
DRIVER_UNLOAD DriverUnload;

_Dispatch_type_(IRP_MJ_DEVICE_CONTROL)
DRIVER_DISPATCH DriverDeviceControl;

_Dispatch_type_(IRP_MJ_SHUTDOWN)
DRIVER_DISPATCH DriverShutdown;

_Dispatch_type_(IRP_MJ_CREATE)
_Dispatch_type_(IRP_MJ_CLOSE)
DRIVER_DISPATCH DriverDeviceCreateClose;

NTSTATUS 
DsCltGetShieldState(
    _In_ PIRP Irp,
    _In_ PIO_STACK_LOCATION IrpStack
    );

NTSTATUS
DsCtlShieldControl(
    _In_ PIRP Irp,
    _In_ PIO_STACK_LOCATION IrpStack
    );

NTSTATUS
DsCtlMeltdownExpose(
    _In_ PIRP Irp,
    _In_ PIO_STACK_LOCATION IrpStack
    );

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

NTSTATUS
DsOpenPolicyKey(
    _Out_ PHANDLE PolicyKey
    );

VOID
DsClosePolicyKey(
    _In_ HANDLE PolicyKey
    );

NTSTATUS
DsQueryLoadModePolicy(
    _Out_ PULONG LoadMode
    );

NTSTATUS
DsSetLoadModePolicy(
    _In_ ULONG LoadMode
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

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, DsCltGetShieldState)
#pragma alloc_text(PAGE, DsCtlShieldControl)
#pragma alloc_text(PAGE, DsCtlMeltdownExpose)
#pragma alloc_text(PAGE, DsAllocateUnicodeString)
#pragma alloc_text(PAGE, DsFreeUnicodeString)
#pragma alloc_text(PAGE, DsCloneUnicodeString)
#pragma alloc_text(PAGE, DsOpenPolicyKey)
#pragma alloc_text(PAGE, DsClosePolicyKey)
#pragma alloc_text(PAGE, DsQueryLoadModePolicy)
#pragma alloc_text(PAGE, DsSetLoadModePolicy)
#if (NTDDI_VERSION >= NTDDI_VISTA) && defined(_WIN64)
#pragma alloc_text(PAGE, DsCheckHvciCompliance)
#endif
#endif

#ifdef DBG
DECLARE_CONST_UNICODE_STRING(
    SDDL_DEVOBJ_SYS_ALL_ADM_RW,
    L"D:P(A;;GA;;;SY)(A;;GRGW;;;BA)"
    );
#endif

KDPC ExposeDpc;
PMM_MAP_IO_SPACE_EX DsMmMapIoSpaceEx;
KTIMER ExposeTimer;
PCHAR LeakBuffer;

PCHAR LeakData = "Overconfidence can take over and caution can be thrown to the wind";

NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT driverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    PDEVICE_OBJECT deviceObject = NULL;
    PCUNICODE_STRING DeviceSecurityString;
    UNICODE_STRING FunctionName;
    ULONG LoadMode = 0;
    BOOLEAN ShutdownCallback = FALSE;
    BOOLEAN SymbolicLink = FALSE;

    gShutdownCalled = FALSE;
    gStateFlags = 0;

#if (NTDDI_VERSION >= NTDDI_VISTA)
	#pragma warning(disable:4055)
    ExInitializeDriverRuntime( DrvRtPoolNxOptIn );

    if (RtlIsNtDdiVersionAvailable( NTDDI_WIN10 )) {

        RtlInitUnicodeString( &FunctionName, L"MmMapIoSpaceEx" );
        DsMmMapIoSpaceEx = (PMM_MAP_IO_SPACE_EX)
                      MmGetSystemRoutineAddress( &FunctionName );

    } else {
        DsMmMapIoSpaceEx = NULL;
    }
#endif
    
    Status = DsCloneUnicodeString( &gDriverKeyName, RegistryPath );

    if (!NT_SUCCESS( Status )) {
        return Status;
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
    Status = IoCreateDeviceSecure( driverObject,
                                   0,
                                   (PUNICODE_STRING) &DsDeviceName,
                                   FILE_DEVICE_UNKNOWN,
                                   FILE_DEVICE_SECURE_OPEN,
                                   FALSE,
                                   DeviceSecurityString,
                                   NULL,
                                   &deviceObject );

    if (!NT_SUCCESS( Status )) {
        goto RoutineExit;
    }

    driverObject->MajorFunction[IRP_MJ_CREATE] = DriverDeviceCreateClose;
    driverObject->MajorFunction[IRP_MJ_CLOSE] = DriverDeviceCreateClose;
    driverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DriverDeviceControl;
    driverObject->MajorFunction[IRP_MJ_SHUTDOWN] = DriverShutdown;
    driverObject->DriverUnload = DriverUnload;

    Status = IoRegisterShutdownNotification( driverObject->DeviceObject );

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
            IoUnregisterShutdownNotification( driverObject->DeviceObject );
        }

        if (deviceObject) {
            IoDeleteDevice( driverObject->DeviceObject );
        }

        DsFreeUnicodeString( &gDriverKeyName );
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

    IoDeleteSymbolicLink( (PUNICODE_STRING) &DsDosDeviceName );
    IoDeleteDevice( DriverObject->DeviceObject );
    DsFreeUnicodeString( &gDriverKeyName );
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

        case IOCTL_MELTDOWN_EXPOSE:
        {
            Status = DsCtlMeltdownExpose( Irp, IrpStack );
            break;
        }
    }

    if (STATUS_PENDING != Status) {
        Irp->IoStatus.Status = Status;
        IoCompleteRequest( Irp, IO_NO_INCREMENT );
    }

    return Status;
}

NTSTATUS
DsCltGetShieldState(
    _In_ PIRP Irp,
    _In_ PIO_STACK_LOCATION IrpStack
    )
{
    PSHIELD_STATUS_DATA StatusData;
    ULONG InputLength;
    ULONG OutputLength;

    PAGED_CODE();

    InputLength  = IrpStack->Parameters.DeviceIoControl.InputBufferLength;
    OutputLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;

    if (max( InputLength, OutputLength ) < sizeof( SHIELD_STATUS_DATA )) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    StatusData = (PSHIELD_STATUS_DATA)Irp->AssociatedIrp.SystemBuffer;
    Irp->IoStatus.Information = sizeof( SHIELD_STATUS_DATA );
    
    if (DsIsShieldRunning()) {
        StatusData->Mode = ShieldRunning;
    }
    else {
        StatusData->Mode = ShieldStopped;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
DsCtlShieldControl(
    _In_ PIRP Irp,
    _In_ PIO_STACK_LOCATION IrpStack
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PSHIELD_CONTROL_DATA ControlData;
    ULONG InputLength;
    ULONG OutputLength;

    PAGED_CODE();

    InputLength = IrpStack->Parameters.DeviceIoControl.InputBufferLength;
    OutputLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;

    if (max( InputLength, OutputLength ) < sizeof( SHIELD_CONTROL_DATA )) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    ControlData = (PSHIELD_CONTROL_DATA)Irp->AssociatedIrp.SystemBuffer;
    ControlData->Result = 0;

    switch (ControlData->Action)
    {
        case ShieldStart: 
        {
            if (DsIsShieldRunning()) {
                ControlData->Result = 0xFF;
                    
                Status = STATUS_SUCCESS;
                break;
            }

            NT_ASSERT( !FlagOn( gStateFlags, DSH_GFL_SHIELD_STARTED ) );

            Status = DsStartShield();
            if (NT_SUCCESS( Status )) {

                SetFlag( gStateFlags, DSH_GFL_SHIELD_STARTED );
                DsSetLoadModePolicy( DSH_RUN_MODE_AUTO_START );
            }

            break;
        }
        case ShieldStop:
        {
            if (!DsIsShieldRunning()) {
                ControlData->Result = 0xFF;
                    
                Status = STATUS_SUCCESS;
                break;
            }

            NT_ASSERT( FlagOn( gStateFlags, DSH_GFL_SHIELD_STARTED ) );

            Status = DsStopShield();

            if (NT_SUCCESS( Status )) {
                ClearFlag( gStateFlags, DSH_GFL_SHIELD_STARTED );
                DsSetLoadModePolicy( DSH_RUN_MODE_DISABLED );
            }

            break;
        }

        default:
        {
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }
    }

    if (NT_SUCCESS( Status )) {
        Irp->IoStatus.Information = sizeof( SHIELD_CONTROL_DATA );
    }

    return Status;
}

NTSTATUS
DsCtlMeltdownExpose(
    _In_ PIRP Irp,
    _In_ PIO_STACK_LOCATION IrpStack
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PMELTDOWN_EXPOSE_DATA ExposeData;
    LARGE_INTEGER DueTime;
    ULONG InputLength;
    ULONG OutputLength;

    PAGED_CODE();

    InputLength = IrpStack->Parameters.DeviceIoControl.InputBufferLength;
    OutputLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;

    if (max( InputLength, OutputLength ) < sizeof( MELTDOWN_EXPOSE_DATA )) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    ExposeData = (PMELTDOWN_EXPOSE_DATA)Irp->AssociatedIrp.SystemBuffer;
    ExposeData->LeakAddress = (UINT64)
                        ExAllocatePoolWithTag( NonPagedPool,
                                               PAGE_SIZE,
                                               'dMsD');

    if (0 == ExposeData->LeakAddress) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    LeakBuffer = (PCHAR)(ULONG_PTR)ExposeData->LeakAddress;
    RtlZeroMemory( LeakBuffer, PAGE_SIZE );
    RtlCopyMemory( LeakBuffer, LeakData, sizeof( LeakData ) );

    DueTime.QuadPart = -1000000;

    //
    //  Fire the L1 periodic 10ms prefetcher.
    //
    KeSetTimerEx( &ExposeTimer, DueTime, 10, &ExposeDpc );

    Irp->IoStatus.Information = sizeof( MELTDOWN_EXPOSE_DATA );
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

NTSTATUS
DsOpenPolicyKey(
    _Out_ PHANDLE PolicyKey
    )
{
    HANDLE RootKey = NULL;
    OBJECT_ATTRIBUTES KeyAttributes;
    UNICODE_STRING KeyName;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    PAGED_CODE();

    NT_ASSERT( PolicyKey );
    *PolicyKey = NULL;

    if (NULL == gDriverKeyName.Buffer) {
        goto RoutineExit;
    }

    InitializeObjectAttributes( &KeyAttributes,
                                &gDriverKeyName,
                                OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                NULL,
                                NULL );

    Status = ZwOpenKey( &RootKey, KEY_ALL_ACCESS, &KeyAttributes );

    if (NT_SUCCESS( Status )) {

        RtlInitUnicodeString( &KeyName, DSH_POLICY_KEY_NAME);

        RtlZeroMemory( &KeyAttributes, sizeof( OBJECT_ATTRIBUTES ));
        InitializeObjectAttributes( &KeyAttributes,
                                    &KeyName,
                                    OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                    RootKey,
                                    NULL);

        Status = ZwCreateKey( PolicyKey,
                              KEY_ALL_ACCESS,
                              &KeyAttributes,
                              0,
                              NULL,
                              REG_OPTION_NON_VOLATILE,
                              NULL );

        if (!NT_SUCCESS( Status )) {
            *PolicyKey = NULL;
        }
    }

RoutineExit:

    if (RootKey) {
        ZwClose( RootKey );
    }

    return Status;
}

VOID
DsClosePolicyKey(
    _In_ HANDLE PolicyKey
    )
{
    PAGED_CODE();
    ZwClose( PolicyKey );
}

NTSTATUS
DsSetLoadModePolicy(
    _In_ ULONG LoadMode
    )
{
    HANDLE PolicyKey;
    UNICODE_STRING ValueName;
    NTSTATUS Status;

    PAGED_CODE();

    Status = DsOpenPolicyKey( &PolicyKey );

    if (NT_SUCCESS( Status )) {
        RtlInitUnicodeString( &ValueName, DSH_RUN_MODE_POLICY );

        Status = ZwSetValueKey( PolicyKey,
                                &ValueName,
                                0,
                                REG_DWORD,
                                &LoadMode,
                                sizeof( ULONG ));

        DsClosePolicyKey( PolicyKey );
    }

    return Status;
}

NTSTATUS
DsQueryLoadModePolicy(
    _Out_ PULONG LoadMode
    )
{
    NTSTATUS Status;
    HANDLE PolicyKey;
    UCHAR Buffer[sizeof( KEY_VALUE_PARTIAL_INFORMATION ) + sizeof( LONG )];
    PKEY_VALUE_PARTIAL_INFORMATION ValuePartialInfo;
    UNICODE_STRING ValueName;
    ULONG ResultLength;

    PAGED_CODE();

    NT_ASSERT( LoadMode );
    *LoadMode = 0;

    Status = DsOpenPolicyKey( &PolicyKey );

    if (NT_SUCCESS( Status )) {
        RtlInitUnicodeString( &ValueName, DSH_RUN_MODE_POLICY );

        Status = ZwQueryValueKey( PolicyKey,
                                  &ValueName,
                                  KeyValuePartialInformation,
                                  Buffer,
                                  sizeof( Buffer ),
                                  &ResultLength );

        if (NT_SUCCESS( Status )) {
            ValuePartialInfo = (PKEY_VALUE_PARTIAL_INFORMATION) Buffer;

            if (ValuePartialInfo->Type == REG_DWORD) {
                *LoadMode = *((PLONG)&(ValuePartialInfo->Data));
            }
        }

        DsClosePolicyKey( PolicyKey );
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
