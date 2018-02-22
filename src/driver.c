#include <ntddk.h>
#include <wdmsec.h>
#include "hv.h"

static UNICODE_STRING gDeviceName;
static UNICODE_STRING gDosDeviceName;
static BOOLEAN        gIsShutdown;

#define DS_WINNT_DEVICE_NAME L"\\Device\\DeepShield"
#define DS_MSDOS_DEVICE_NAME L"\\DosDevices\\DeepShield"

VOID
DriverUnload(
    _In_ PDRIVER_OBJECT driverObject
);

NTSTATUS
DriverShutdown(
    _In_    PDEVICE_OBJECT deviceObject,
    _Inout_ PIRP           irp
);

NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT driverObject,
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

    driverObject->MajorFunction[IRP_MJ_SHUTDOWN] = DriverShutdown;

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
        HvStop();
        HvDone();
    }

    IoDeleteSymbolicLink( &gDosDeviceName );
    IoUnregisterShutdownNotification( driverObject->DeviceObject );
    IoDeleteDevice( driverObject->DeviceObject );
}

NTSTATUS
DriverShutdown(
    _In_ PDEVICE_OBJECT deviceObject,
    _Inout_ PIRP irp
)
{
    UNREFERENCED_PARAMETER(deviceObject);

    gIsShutdown = TRUE;
    
    HvStop();
    HvDone();

    irp->IoStatus.Status = STATUS_SUCCESS;
    irp->IoStatus.Information = 0;
    IoCompleteRequest( irp, IO_NO_INCREMENT );

    return STATUS_SUCCESS;
}
