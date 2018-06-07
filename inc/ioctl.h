#ifndef __SHIELD_IOCTL_H__
#define __SHIELD_IOCTL_H__

#include <ntifs.h>
#include "wdk7.h"

#define DS_WINNT_DEVICE_NAME L"\\Device\\DeepShield"
#define DS_MSDOS_DEVICE_NAME L"\\DosDevices\\DeepShield"

#define IOCTL_SHIELD_TYPE 0xB080

//
// Status IOCTL
//
#define IOCTL_SHIELD_STATUS \
    CTL_CODE( IOCTL_SHIELD_TYPE, 0x0A00, METHOD_BUFFERED, FILE_READ_ACCESS )

typedef enum _SHIELD_STATUS_MODE
{
    ShieldStopped = 1,
    ShieldRunning
} SHIELD_STATUS_MODE, *PSHIELD_STATUS_MODE;

typedef struct _SHIELD_STATUS_DATA
{
    SHIELD_STATUS_MODE Mode;
} SHIELD_STATUS_DATA, *PSHIELD_STATUS_DATA;

//
// Control IOCTL
//
#define IOCTL_SHIELD_CONTROL \
    CTL_CODE( IOCTL_SHIELD_TYPE, 0x0A01, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS  )

typedef enum _SHIELD_CONTROL_ACTION
{
    ShieldStop = 1,
    ShieldStart
} SHIELD_CONTROL_ACTION, *PSHIELD_CONTROL_ACTION;

typedef struct _SHIELD_CONTROL_DATA
{
    SHIELD_CONTROL_ACTION Action;
    ULONG Result;
} SHIELD_CONTROL_DATA, *PSHIELD_CONTROL_DATA;

#define IOCTL_SHIELD_CHANNEL_SETUP \
    CTL_CODE( IOCTL_SHIELD_TYPE, 0x0A100, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS  )

#define IOCTL_SHIELD_CHANNEL_TEARDOWN \
    CTL_CODE( IOCTL_SHIELD_TYPE, 0x0A101, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS  )

typedef struct _SHIELD_CHANNEL_DATA {
    ULONG ChannelId;
    PVOID ChannelAddress;
    ULONG ChannelSize;
} SHIELD_CHANNEL_DATA, *PSHIELD_CHANNEL_DATA;

typedef struct _SHIELD_CHANNEL_ID {
    ULONG ChannelId;
} SHIELD_CHANNEL_ID, *PSHIELD_CHANNEL_ID;

//
// Meltdown Expose IOCTL
//
#define IOCTL_MELTDOWN_EXPOSE \
    CTL_CODE( IOCTL_SHIELD_TYPE, 0x0A02, METHOD_BUFFERED, FILE_ANY_ACCESS )

#define IOCTL_MELTDOWN_COVER \
    CTL_CODE( IOCTL_SHIELD_TYPE, 0x0A03, METHOD_BUFFERED, FILE_ANY_ACCESS )

// 
// 
// TESTS of RDTSC Detection
//
#define IOCTL_TEST_RDTSC \
    CTL_CODE( IOCTL_SHIELD_TYPE, 0x0A20, METHOD_BUFFERED, FILE_ANY_ACCESS )

typedef struct _MELTDOWN_EXPOSE_DATA
{
    ULONG64 LeakAddress;
} MELTDOWN_EXPOSE_DATA, *PMELTDOWN_EXPOSE_DATA;

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
DsCtlShieldChannelSetup(
    _In_ PIRP Irp,
    _In_ PIO_STACK_LOCATION IrpStack
    );

NTSTATUS
DsCtlShieldChannelTeardown(
    _In_ PIRP Irp,
    _In_ PIO_STACK_LOCATION IrpStack
    );

NTSTATUS
DsCtlMeltdownExpose(
    _In_ PIRP Irp,
    _In_ PIO_STACK_LOCATION IrpStack
    );

#endif
