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

typedef struct _MAILBOX {
    PUCHAR PoolBuffer;
    RING_BUFFER RingBuffer;
    PETHREAD Thread;
    KEVENT ShutdownEvent;
    KSEMAPHORE QueueSemaphore;
    PKSTART_ROUTINE WorkerThread;
} MAILBOX, *PMAILBOX;

typedef enum _MAILBOX_TYPE {
    MailboxEmpty,
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
RtlMailboxStartWorker(
    _Inout_ PMAILBOX Mailbox
    );

VOID
RtlMailboxStopWorker(
    _Inout_ PMAILBOX Mailbox
    );

VOID
RtlMailboxWorkerThread(
    _In_ PVOID Context
);

NTSTATUS
RtlMailboxInitialize(
    _Inout_ PMAILBOX Mailbox,
    _In_ ULONG PoolSize
    );

VOID
RtlMailboxDestroy(
    _Inout_ PMAILBOX Mailbox
    );

NTSTATUS
RtlPostMailboxTrace(
    _Inout_ PMAILBOX Mailbox,
    _In_ ULONG Level,
    _In_ ULONG Area,
    __drv_formatString( printf ) _In_ PCSTR TraceMessage,
    ...
    );

NTSTATUS
RtlPostMailboxNotification(
    _Inout_ PMAILBOX Mailbox,
    _In_ PVOID Notification,
    _In_ SIZE_T Length
    );

NTSTATUS
RtlRetrieveMailboxData(
    _Inout_ PMAILBOX Mailbox,
    _Inout_ PMAILBOX_HEADER MailboxHeader,
    _Out_writes_bytes_( DataSize ) PCHAR Data,
    _In_ SIZE_T DataSize
    );

#endif