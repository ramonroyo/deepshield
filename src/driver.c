#include <ntddk.h>
#include "hv.h"

#include "vmx.h"
#include "x86.h"

static UNICODE_STRING gDeviceName;
static UNICODE_STRING gDosDeviceName;
//static BOOLEAN        gIsShutdown;

#define WINNT_DEVICE_NAME L"\\Device\\antimelt"
#define MSDOS_DEVICE_NAME L"\\DosDevices\\antimelt"

VOID
DriverUnload(
    _In_ PDRIVER_OBJECT driverObject
);

/*
NTSTATUS
DriverShutdown(
    _In_    PDEVICE_OBJECT deviceObject,
    _Inout_ PIRP           irp
);
*/

NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT  driverObject,
    _In_ PUNICODE_STRING registryPath
)
{
    NTSTATUS       status           = STATUS_UNSUCCESSFUL;
    PDEVICE_OBJECT deviceObject     = NULL;
//    BOOLEAN        shutdownCallback = FALSE;
    BOOLEAN        symbolicLink     = FALSE;
    BOOLEAN        hvInitiated      = FALSE;

	UNREFERENCED_PARAMETER(registryPath);

    RtlInitUnicodeString( &gDeviceName, WINNT_DEVICE_NAME );
    RtlInitUnicodeString( &gDosDeviceName, MSDOS_DEVICE_NAME );
//    gIsShutdown = FALSE;

    //
    //
    //
    status = IoCreateDevice( driverObject,
                             0,
                             &gDeviceName,
                             FILE_DEVICE_UNKNOWN,
                             0,
                             FALSE,
                             &deviceObject );
    if (!NT_SUCCESS(status))
    {
        return status;
    }
/*
    //
    //
    //
    driverObject->MajorFunction[IRP_MJ_SHUTDOWN] = DriverShutdown;

    status = IoRegisterShutdownNotification(driverObject->DeviceObject);
    if (!NT_SUCCESS(status))
    {
        goto RoutineExit;
    }
    shutdownCallback = TRUE;
*/
    //
    //
    //
    status = IoCreateSymbolicLink( &gDosDeviceName, &gDeviceName );
    if (!NT_SUCCESS(status))
    {
        goto RoutineExit;
    }
    symbolicLink = TRUE;

    //
    //
    //
    status = HvInit();
    if (!NT_SUCCESS(status))
    {
        goto RoutineExit;
    }
    hvInitiated = TRUE;

    //
    //
    //
    status = HvStart();
    if (!NT_SUCCESS(status))
    {
        goto RoutineExit;
    }

    //
    //
    //
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
/*
        if (shutdownCallback)
        {
            IoUnregisterShutdownNotification( driverObject->DeviceObject );
        }
*/
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
//    if (!gIsShutdown)
//    {
        HvStop();
        HvDone();
//    }

    IoDeleteSymbolicLink( &gDosDeviceName );
    IoUnregisterShutdownNotification( driverObject->DeviceObject );
    IoDeleteDevice( driverObject->DeviceObject );
}

/*
NTSTATUS
DriverShutdown(
    _In_    PDEVICE_OBJECT deviceObject,
    _Inout_ PIRP           irp
)
{
    UNREFERENCED_PARAMETER(deviceObject);

    gIsShutdown = TRUE;
    
    HvStop();
    HvDone();

    irp->IoStatus.Status = STATUS_SUCCESS;
    irp->IoStatus.Information = 0;
    IoCompleteRequest(irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}
*/