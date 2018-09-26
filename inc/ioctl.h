#ifndef __SHIELD_IOCTL_H__
#define __SHIELD_IOCTL_H__

#include <ntifs.h>
#include "wdk7.h"
#include "..\src\dsdef.h"

#pragma warning(disable:4201)   // nameless struct/union

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

#define IOCTL_SHIELD_GET_VMFEATURE \
    CTL_CODE( IOCTL_SHIELD_TYPE, 0x0A201, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS  )

#define VM_STATE_SIZE    (512)
typedef struct _SHIELD_VM_FEATURE {

    UINT32 Version;

    union {
        struct {
            UINT32 NoIntelCpu : 1;
            UINT32 HvciEnabled : 1;
            UINT32 NoVtxExtension : 1;
            UINT32 FirmwareDisabled : 1;
            UINT32 Rsvd4To31 : 28;
        };

        UINT32 AsUint32;
    } Bits;

    UCHAR VmStateBlob[VM_STATE_SIZE];
} SHIELD_VM_FEATURE, *PSHIELD_VM_FEATURE;

// 
//  Tests for RDTSC Detection
//
#define IOCTL_TEST_RDTSC \
    CTL_CODE( IOCTL_SHIELD_TYPE, 0x0A20, METHOD_BUFFERED, FILE_ANY_ACCESS )

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
DsCtlShieldGetVmFeature(
    _In_ PIRP Irp,
    _In_ PIO_STACK_LOCATION IrpStack
    );

#endif
