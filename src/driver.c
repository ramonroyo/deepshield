#include "wdk7.h"
#include <ntddk.h>
#include <wdmsec.h>
#include <ioctl.h>
#include "hv.h"

static UNICODE_STRING gDeviceName;
static UNICODE_STRING gDosDeviceName;
static BOOLEAN        gIsShutdown;

DRIVER_INITIALIZE DriverEntry;
DRIVER_UNLOAD     DriverUnload;

_Dispatch_type_(IRP_MJ_DEVICE_CONTROL)
DRIVER_DISPATCH   DriverDeviceControl;

_Dispatch_type_(IRP_MJ_SHUTDOWN)
DRIVER_DISPATCH   DriverShutdown;

_Dispatch_type_(IRP_MJ_CREATE)
_Dispatch_type_(IRP_MJ_CLOSE)
DRIVER_DISPATCH   DriverDeviceCreateClose;

#ifdef DBG
DECLARE_CONST_UNICODE_STRING(
    SDDL_DEVOBJ_SYS_ALL_ADM_RW,
    L"D:P(A;;GA;;;SY)(A;;GRGW;;;BA)"
    );
#endif

NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT  driverObject,
    _In_ PUNICODE_STRING registryPath
    )
{
    NTSTATUS       status           = STATUS_UNSUCCESSFUL;
    PDEVICE_OBJECT deviceObject     = NULL;
    BOOLEAN        shutdownCallback = FALSE;
    BOOLEAN        symbolicLink     = FALSE;
    BOOLEAN        hvInitiated      = FALSE;
    PCUNICODE_STRING DeviceSecurityString;

    UNREFERENCED_PARAMETER(registryPath);

    RtlInitUnicodeString( &gDeviceName, DS_WINNT_DEVICE_NAME );
    RtlInitUnicodeString( &gDosDeviceName, DS_MSDOS_DEVICE_NAME );

    gIsShutdown = FALSE;

#ifdef DBG
    DeviceSecurityString = &SDDL_DEVOBJ_SYS_ALL_ADM_RW;
#else
    DeviceSecurityString = &SDDL_DEVOBJ_SYS_ALL;
#endif


    //
    //  Kernel code and user mode code running as *SYSTEM* is allowed to open
    //  the device for any access. This tight security settings and let its
    //  service to open the device up.
    //
    status = IoCreateDeviceSecure( driverObject,
                                   0,
                                   &gDeviceName,
                                   FILE_DEVICE_UNKNOWN,
                                   FILE_DEVICE_SECURE_OPEN,
                                   FALSE,
                                   DeviceSecurityString,
                                   NULL,
                                   &deviceObject );

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    driverObject->MajorFunction[IRP_MJ_CREATE]       = DriverDeviceCreateClose;
    driverObject->MajorFunction[IRP_MJ_CLOSE]        = DriverDeviceCreateClose;
    driverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DriverDeviceControl;
    driverObject->MajorFunction[IRP_MJ_SHUTDOWN]       = DriverShutdown;

    status = IoRegisterShutdownNotification(driverObject->DeviceObject);

    if (!NT_SUCCESS(status))
    {
        goto RoutineExit;
    }

    shutdownCallback = TRUE;

    status = IoCreateSymbolicLink( &gDosDeviceName, &gDeviceName );

    if (!NT_SUCCESS(status)) {
        goto RoutineExit;
    }

    symbolicLink = TRUE;

    status = HvInit();

    if (!NT_SUCCESS(status)) {
        goto RoutineExit;
    }

    hvInitiated = TRUE;

    //
    // Activate protection as soon as driver starts
    //
    status = HvStart();

    if (!NT_SUCCESS(status)) {
        goto RoutineExit;
    }

    driverObject->DriverUnload = DriverUnload;

RoutineExit:

    if (!NT_SUCCESS(status))
    {
        if (hvInitiated)
        {
            HvDone();
        }

        if (symbolicLink)
        {
            IoDeleteSymbolicLink( &gDosDeviceName );
        }

        if (shutdownCallback)
        {
            IoUnregisterShutdownNotification( driverObject->DeviceObject );
        }

        if (deviceObject)
        {
            IoDeleteDevice( driverObject->DeviceObject );
        }
    }

    return status;
}

VOID
DriverUnload(
    _In_ PDRIVER_OBJECT driverObject
)
{
    if (!gIsShutdown) {

        if (HvLaunched()) {
            HvStop();
        }

        HvDone();
    }

    IoDeleteSymbolicLink( &gDosDeviceName );
    IoUnregisterShutdownNotification( driverObject->DeviceObject );
    IoDeleteDevice( driverObject->DeviceObject );
}

NTSTATUS
DriverShutdown(
    _In_    PDEVICE_OBJECT deviceObject,
    _Inout_ PIRP           irp
)
{
    UNREFERENCED_PARAMETER(deviceObject);

    gIsShutdown = TRUE;
    
    if (HvLaunched()) {
        HvStop();
    }

    HvDone();

    irp->IoStatus.Status = STATUS_SUCCESS;
    irp->IoStatus.Information = 0;
    IoCompleteRequest( irp, IO_NO_INCREMENT );

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
DeviceStatus(
    _In_ PIRP               irp,
    _In_ PIO_STACK_LOCATION irpStack
);

NTSTATUS
DeviceControl(
    _In_ PIRP               irp,
    _In_ PIO_STACK_LOCATION irpStack
);

NTSTATUS
DriverDeviceControl(
    _In_    PDEVICE_OBJECT deviceObject,
    _Inout_ PIRP           irp
)
{
    NTSTATUS           status = STATUS_SUCCESS;
    PIO_STACK_LOCATION irpStack;

    UNREFERENCED_PARAMETER(deviceObject);

    irpStack = IoGetCurrentIrpStackLocation(irp);

    irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
    irp->IoStatus.Information = 0;

    switch (irpStack->Parameters.DeviceIoControl.IoControlCode)
    {
        case IOCTL_SHIELD_STATUS:
        {
            status = DeviceStatus(irp, irpStack);
            break;
        }
        case IOCTL_SHIELD_CONTROL:
        {
            status = DeviceControl(irp, irpStack);
            break;
        }
    }

    if (STATUS_PENDING != status)
    {
        irp->IoStatus.Status = status;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
    }

    return status;
}

NTSTATUS
DeviceStatus(
    _In_ PIRP               irp,
    _In_ PIO_STACK_LOCATION irpStack
)
{
    PSHIELD_STATUS_DATA data;
    ULONG               inputLength;
    ULONG               outputLength;

    inputLength  = irpStack->Parameters.DeviceIoControl.InputBufferLength;
    outputLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;

    if (max(inputLength, outputLength) < sizeof(SHIELD_STATUS_DATA))
        return STATUS_BUFFER_TOO_SMALL;

    data = (PSHIELD_STATUS_DATA)irp->AssociatedIrp.SystemBuffer;
    irp->IoStatus.Information = sizeof(SHIELD_STATUS_DATA);
    
    if (HvLaunched())
    {
        data->mode = ShieldRunning;
    }
    else
    {
        data->mode = ShieldStopped;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
DeviceControl(
    _In_ PIRP Irp,
    _In_ PIO_STACK_LOCATION IrpStack
    )
{
    NTSTATUS Status;
    PSHIELD_CONTROL_DATA ControlData;
    ULONG InputLength;
    ULONG OutputLength;

    InputLength = IrpStack->Parameters.DeviceIoControl.InputBufferLength;
    OutputLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;

    if (max( InputLength, OutputLength ) < sizeof( SHIELD_CONTROL_DATA )) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    ControlData = (PSHIELD_CONTROL_DATA)Irp->AssociatedIrp.SystemBuffer;
    ControlData->result = STATUS_SUCCESS;

    switch (ControlData->action)
    {
        case ShieldStart: 
        {
            if (HvLaunched()) {
                ControlData->result = STATUS_ALREADY_COMPLETE;
                    
                Status = STATUS_SUCCESS;
                break;
            }
            
            Status = HvStart();
            break;
        }
        case ShieldStop:
        {
             if (!HvLaunched()) {
                ControlData->result = STATUS_ALREADY_COMPLETE;
                    
                Status = STATUS_SUCCESS;
                break;
            }

            Status = HvStop();
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
