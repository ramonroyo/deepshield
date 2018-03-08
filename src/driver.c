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

    UNREFERENCED_PARAMETER(registryPath);

    RtlInitUnicodeString( &gDeviceName, DS_WINNT_DEVICE_NAME );
    RtlInitUnicodeString( &gDosDeviceName, DS_MSDOS_DEVICE_NAME );

    gIsShutdown = FALSE;

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
                                   &SDDL_DEVOBJ_SYS_ALL,
                                   NULL,
                                   &deviceObject );

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    driverObject->MajorFunction[ IRP_MJ_CREATE ]       = DriverDeviceCreateClose;
    driverObject->MajorFunction[ IRP_MJ_CLOSE ]        = DriverDeviceCreateClose;
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
    if (!gIsShutdown)
    {
        if(HvLaunched())
            HvStop();

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
    
    if (HvLaunched())
        HvStop();

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
    
    if (NT_SUCCESS(HvLaunched()))
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
    _In_ PIRP               irp,
    _In_ PIO_STACK_LOCATION irpStack
)
{
    PSHIELD_CONTROL_DATA data;
    ULONG                inputLength;
    ULONG                outputLength;

    inputLength = irpStack->Parameters.DeviceIoControl.InputBufferLength;
    outputLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;

    if (max(inputLength, outputLength) < sizeof(SHIELD_CONTROL_DATA))
        return STATUS_BUFFER_TOO_SMALL;

    data = (PSHIELD_CONTROL_DATA)irp->AssociatedIrp.SystemBuffer;
    irp->IoStatus.Information = sizeof(SHIELD_CONTROL_DATA);

    switch (data->action)
    {
        case ShieldStart: 
        {
            if (NT_SUCCESS(HvLaunched()))
            {
                data->result = STATUS_ALREADY_COMPLETE;
            }
            else
            {
                data->result = HvStart();
            }
            break;
        }
        case ShieldStop:
        {
            if (NT_SUCCESS(HvLaunched()))
            {
                data->result = HvStop();
            }
            else
            {
                data->result = STATUS_ALREADY_COMPLETE;
            }
            break;
        }
    }

    return data->result;
}
