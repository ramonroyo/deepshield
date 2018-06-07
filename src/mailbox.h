/*++

Copyright (C) ByteHeed. All Rights Reserved

Module Name:

    mailbox.h

--*/

#ifndef _MAILBOX_H
#define _MAILBOX_H_

#include <ntifs.h>
#include "wdk7.h"

#define MAILBOX_BUFFER_SIZE        1024
#define MAILBOX_POOL_DEFAULT_SIZE (1024 * 256)

typedef enum _MAILBOX_TYPE {
    MailboxTrace,
    MailboxNotification
} MAILBOX_TYPE;

typedef struct _MAILBOX_HEADER {

    //
    //  Input flags (none currently defined).
    //

    ULONG Flags;
    SIZE_T Size;

    //
    //  Specifies which type of mailbox we are processing and selects which
    //  member of the below union is in use.
    //

    MAILBOX_TYPE Type;

    union {
        struct {
            ULONG Area;
            ULONG Level;
        } Trace;

        struct {
            ULONG Reserved;
        } Notification;

    } DUMMYUNIONNAME;

} MAILBOX_HEADER, *PMAILBOX_HEADER;

NTSTATUS
RtlMailboxStartThread(
    VOID
    );

VOID
RtlMailboxStopThread(
    VOID
    );

VOID
RtlMailboxPollingThread(
    _In_ PVOID Context
);

NTSTATUS
RtlMailboxInitialize(
    _In_ ULONG PoolSize
    );

VOID
RtlMailboxDestroy(
    );

NTSTATUS
RtlPostMailboxTrace(
    _In_ ULONG Level,
    _In_ ULONG Area,
    __drv_formatString( printf ) _In_ PCSTR TraceMessage,
    ...
    );

NTSTATUS
RtlPostMailboxNotification(
    _In_ PVOID Notification,
    _In_ SIZE_T Length
    );

NTSTATUS
RtlRetrieveMailbox(
    _Inout_ PMAILBOX_HEADER Mailbox,
    _Out_writes_bytes_( DataSize ) PCHAR Data,
    _In_ SIZE_T DataSize
    );

#endif