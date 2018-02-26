#include "wdk7.h"
#include <ntddk.h>
#include <wdmsec.h>
#include <ioctl.h>
#include "hv.h"

static UNICODE_STRING gDeviceName;
static UNICODE_STRING gDosDeviceName;
static BOOLEAN        gIsShutdown;
static FAST_MUTEX     gMutex;
static BOOLEAN        gIsDeviceOpen = 0;
static ULONG          gToken        = 0;

//
// From <ntifs.h>
//
#ifndef WDK7
_IRQL_requires_max_(APC_LEVEL)
_Ret_range_(<= , MAXLONG)
NTSYSAPI
#endif
ULONG
NTAPI
RtlRandomEx(
    _Inout_ PULONG Seed
);
//
// 
//


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
DriverDispatchOpenClose(
    _In_    PDEVICE_OBJECT deviceObject,
    _Inout_ PIRP           irp
);

NTSTATUS
DriverDeviceControl(
    _In_    PDEVICE_OBJECT deviceObject,
    _Inout_ PIRP           irp
);

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

    ExInitializeFastMutex(&gMutex);

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

    driverObject->MajorFunction[IRP_MJ_CREATE]         = DriverDispatchOpenClose;
    driverObject->MajorFunction[IRP_MJ_CLOSE]          = DriverDispatchOpenClose;
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

    ExReleaseFastMutex(&gMutex);
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

    ExAcquireFastMutex(&gMutex);

    gIsDeviceOpen = FALSE;
    gToken = 0;

    ExReleaseFastMutex(&gMutex);
    
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
DriverDispatchOpenClose(
    _In_    PDEVICE_OBJECT deviceObject,
    _Inout_ PIRP           irp
)
{
    NTSTATUS           status = STATUS_INVALID_DEVICE_REQUEST;
    PIO_STACK_LOCATION irpStack;

    UNREFERENCED_PARAMETER(deviceObject);

    irpStack = IoGetCurrentIrpStackLocation(irp);

    ExAcquireFastMutex(&gMutex);

    switch (irpStack->MajorFunction)
    {
        case IRP_MJ_CREATE:
        {
            if (!gIsDeviceOpen)
            {
                ULONG length;
                PVOID buffer;

                length = irpStack->Parameters.DeviceIoControl.OutputBufferLength;
                buffer = irp->AssociatedIrp.SystemBuffer;

                if (length >= sizeof(ULONG))
                {
                    LARGE_INTEGER performanceCounter = KeQueryPerformanceCounter(0);
                    gToken = RtlRandomEx(&performanceCounter.LowPart);
                    
                    *(PULONG)buffer = gToken;

                    gIsDeviceOpen = TRUE;
                    status = STATUS_SUCCESS;
                }
                else
                {
                    status = STATUS_BUFFER_TOO_SMALL;
                }
            }
            break;
        }
        case IRP_MJ_CLOSE:
        {
            if (gIsDeviceOpen)
            {
                ULONG length;
                PVOID buffer;

                length = irpStack->Parameters.DeviceIoControl.OutputBufferLength;
                buffer = irp->AssociatedIrp.SystemBuffer;

                if (length >= sizeof(ULONG))
                {
                    if (*(PULONG)buffer == gToken)
                    {
                        gToken = 0;
                        gIsDeviceOpen = FALSE;
                        status = STATUS_SUCCESS;
                    }
                }
                else
                {
                    status = STATUS_BUFFER_TOO_SMALL;
                }
            }
            break;
        }
    }

    ExReleaseFastMutex(&gMutex);

    irp->IoStatus.Status      = status;
    irp->IoStatus.Information = 0;

    IoCompleteRequest(irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS
DriverDeviceControl(
    _In_    PDEVICE_OBJECT deviceObject,
    _Inout_ PIRP           irp
)
{
    NTSTATUS           status = STATUS_SUCCESS;
    PIO_STACK_LOCATION irpStack;
    ULONG              controlCode;
    ULONG              length;
    PVOID              buffer;

    UNREFERENCED_PARAMETER(deviceObject);

    irpStack = IoGetCurrentIrpStackLocation(irp);
    controlCode = irpStack->Parameters.DeviceIoControl.IoControlCode;

    irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
    irp->IoStatus.Information = 0;

    ExAcquireFastMutex(&gMutex);

    if (!gIsDeviceOpen)
    {
        ExReleaseFastMutex(&gMutex);
        goto end;
    }

    length = irpStack->Parameters.DeviceIoControl.InputBufferLength;
    buffer = irp->AssociatedIrp.SystemBuffer;

    if (length < sizeof(ULONG))
    {
        status = STATUS_BUFFER_TOO_SMALL;
        ExReleaseFastMutex(&gMutex);
        goto end;
    }

    if (*(PULONG)buffer != gToken)
    {
        ExReleaseFastMutex(&gMutex);
        goto end;
    }

    switch (controlCode)
    {
        case IOCTL_SHIELD_START_PROTECTION:
        {
            status = HvStart();
            break;
        }
        case IOCTL_SHIELD_STOP_PROTECTION:
        {
            status = HvStop();
            break;
        }
        case IOCTL_SHIELD_PROTECTION_STATUS:
        {
            status = HvLaunched();
            break;
        }
    }

    ExReleaseFastMutex(&gMutex);

end:
    if (STATUS_PENDING != status)
    {
        irp->IoStatus.Status = status;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
    }

    return status;
}
