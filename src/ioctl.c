/*++

Copyright (c) ByteHeed. All Rights Reserved.

Module Name:

    ioctl.c

Abstract:

    This file implements the IOCTL handlers.

Environment:

--*/

#include "dsdef.h"

#if defined(WPP_EVENT_TRACING)
#include "ioctl.tmh"
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, DsCltGetShieldState)
#pragma alloc_text(PAGE, DsCtlShieldControl)
#pragma alloc_text(PAGE, DsCtlMeltdownExpose)
#pragma alloc_text(PAGE, DsCtlTestRdtscDetection)
#pragma alloc_text(PAGE, DsCtlShieldChannelSetup)
#pragma alloc_text(PAGE, DsCtlShieldChannelTeardown)
#endif

extern KDPC ExposeDpc;
extern KTIMER ExposeTimer;
extern PCHAR LeakBuffer;

#define LEAKDATA_LENGTH 67
UCHAR LeakData[ LEAKDATA_LENGTH ] = "Overconfidence can take over and caution can be thrown to the wind";

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

                if (FlagOn( gStateFlags, DSH_GFL_CHANNEL_SETUP )) {
                    //
                    //  Close the gate and wait for any ongoing channel
                    //  reference.
                    //
                    ClearFlag( gStateFlags, DSH_GFL_CHANNEL_SETUP );

                    ExWaitForRundownProtectionRelease( &gChannelRundown );
                    ExRundownCompleted( &gChannelRundown );

                    DsDestroyChannel( gChannel, StopShieldReason );
                }
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
DsCtlShieldChannelSetup(
    _In_ PIRP Irp,
    _In_ PIO_STACK_LOCATION IrpStack
    )
{
    NTSTATUS Status = STATUS_INVALID_DEVICE_REQUEST;
    PSHIELD_CHANNEL_DATA ChannelData;
    ULONG OutputLength;
    static BOOLEAN FirstUse = TRUE;

    PAGED_CODE();

    OutputLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;

    if (OutputLength < sizeof( SHIELD_CHANNEL_DATA )) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    if (FALSE == DsIsShieldRunning() 
        || FlagOn( gStateFlags, DSH_GFL_CHANNEL_SETUP )) {
        return STATUS_INVALID_PARAMETER;
    }

    ChannelData = (PSHIELD_CHANNEL_DATA)Irp->AssociatedIrp.SystemBuffer;

    Status = DsInitializeChannel( DS_MAX_BUCKETS, &gChannel );

    if (NT_SUCCESS( Status ) ) {

        if (FirstUse) {
            ExInitializeRundownProtection( &gChannelRundown );
            FirstUse = FALSE;
        }
        else {
            ExReInitializeRundownProtection( &gChannelRundown );
        }

        SetFlag( gStateFlags, DSH_GFL_CHANNEL_SETUP );

        ChannelData->ChannelId = gChannel->ChannelId;
        ChannelData->ChannelAddress = gChannel->UserVa;
        ChannelData->ChannelSize = gChannel->UserSize;

        Irp->IoStatus.Information = sizeof( SHIELD_CHANNEL_DATA );
    }

    return Status;
}

NTSTATUS
DsCtlShieldChannelTeardown(
    _In_ PIRP Irp,
    _In_ PIO_STACK_LOCATION IrpStack
    )
{
    NTSTATUS Status = STATUS_INVALID_DEVICE_REQUEST;
    PSHIELD_CHANNEL_ID ChannelId;
    ULONG InputLength;
    ULONG OutputLength;

    PAGED_CODE();

    InputLength = IrpStack->Parameters.DeviceIoControl.InputBufferLength;
    OutputLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;

    if (max( InputLength, OutputLength ) < sizeof( SHIELD_CHANNEL_ID )) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    if (!FlagOn( gStateFlags, DSH_GFL_CHANNEL_SETUP )) {
        return STATUS_INVALID_PARAMETER;
    }

    ChannelId = (PSHIELD_CHANNEL_ID)Irp->AssociatedIrp.SystemBuffer;

    if (ChannelId->ChannelId != gChannel->ChannelId) {
        return STATUS_INVALID_PARAMETER;
    }

    ClearFlag( gStateFlags, DSH_GFL_CHANNEL_SETUP );

    //
    //  Close the gate and wait for any ongoing channel reference.
    //

    ExWaitForRundownProtectionRelease( &gChannelRundown );
    ExRundownCompleted( &gChannelRundown );

    DsDestroyChannel( gChannel, TeardownReason );

    Irp->IoStatus.Information = 0;
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
    RtlCopyMemory( LeakBuffer, LeakData, LEAKDATA_LENGTH );

    DueTime.QuadPart = -1000000;

    //
    //  Fire the L1 periodic 10ms prefetcher.
    //
    KeSetTimerEx( &ExposeTimer, DueTime, 10, &ExposeDpc );

    Irp->IoStatus.Information = sizeof( MELTDOWN_EXPOSE_DATA );
    return Status;
}