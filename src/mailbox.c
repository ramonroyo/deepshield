/*++

Copyright (c) ByteHeed. All Rights Reserved.

Module Name:

    mailbox.c

Abstract:

    This file implements the mailbox utility to communicate messages with
    root mode.

Environment:

--*/

#include "dsdef.h"

#if defined(WPP_EVENT_TRACING)
#include "mailbox.tmh"
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RtlMailboxStartThread)
#pragma alloc_text(PAGE, RtlMailboxStopThread)
#pragma alloc_text(PAGE, RtlMailboxInitialize)
#pragma alloc_text(PAGE, RtlMailboxDestroy)
#pragma alloc_text(PAGE, RtlMailboxPollingThread)
#endif

static PUCHAR TracePool;
static RING_BUFFER TraceRing;
static PVOID ThreadObject;

static KEVENT ShutdownEvent;
static KSEMAPHORE MailboxQueueSemaphore;

KSTART_ROUTINE RtlMailboxPollingThread;

VOID
RtlMailboxPollingThread(
    _In_ PVOID Context
);

NTSTATUS
RtlMailboxStartThread(
    VOID
    )
{
    NTSTATUS Status;
    HANDLE Thread;

    PAGED_CODE();

    KeInitializeEvent( &ShutdownEvent, NotificationEvent, FALSE );
    KeInitializeSemaphore( &MailboxQueueSemaphore, 0, MAXLONG );
    
    Status = PsCreateSystemThread( &Thread, 
                                   THREAD_ALL_ACCESS, 
                                   NULL,
                                   NULL,
                                   NULL,
                                   RtlMailboxPollingThread,
                                   NULL );

    if (NT_SUCCESS( Status )) {
        ObReferenceObjectByHandle( Thread,
                                   THREAD_ALL_ACCESS,
                                   NULL,
                                   KernelMode, 
                                   (PVOID*)ThreadObject,
                                   NULL );
        ZwClose( Thread );
    }

    return Status;
}

VOID 
RtlMailboxStopThread(
    VOID
    )
{
    PAGED_CODE();
    KeSetEvent( &ShutdownEvent, IO_NO_INCREMENT, FALSE );

    KeWaitForSingleObject( ThreadObject, Executive, KernelMode, FALSE, NULL );
    ObDereferenceObject( ThreadObject );
}

VOID
RtlMailboxPollingThread(
    _In_ PVOID Context
    )
{
    NTSTATUS Status;
    CHAR Data[1024];
    PVOID WaitObjects[] = { &ShutdownEvent, &MailboxQueueSemaphore };
    MAILBOX_HEADER Mailbox;
    PDS_NOTIFICATION_MESSAGE Message;

    PAGED_CODE();
    UNREFERENCED_PARAMETER( Context );

    KeSetPriorityThread( KeGetCurrentThread(), LOW_REALTIME_PRIORITY );

    for (;;) {
        Status = KeWaitForMultipleObjects( 2,
                                           WaitObjects,
                                           WaitAny,
                                           Executive,
                                           KernelMode,
                                           FALSE,
                                           NULL,
                                           NULL );

        if (Status == STATUS_WAIT_0) {
            PsTerminateSystemThread( STATUS_SUCCESS );
        }
        else if (Status == STATUS_WAIT_1) {
            Status = RtlRetrieveMailbox( &Mailbox, Data, 1024 );

            if (!NT_SUCCESS( Status )) {
                NT_ASSERT( FALSE );
            }

            if (Mailbox.Type == MailboxTrace) {
                if (Mailbox.Trace.Area == TRACE_IOA_ROOT) {
                    TraceEvents( TRACE_LEVEL_INFORMATION,
                                 TRACE_IOA,
                                 "%s\n",
                                 Data );
                }
            }
            else if (Mailbox.Type == MailboxNotification) {
                Message = (PDS_NOTIFICATION_MESSAGE) Data;

                Status = DsSendNotificationMessage( gChannel,
                                                    Message->ProcessId,
                                                    Message->ThreadId,
                                                    Message->Type,
                                                    &Message->Action );
            }
        }
    }
}

NTSTATUS
RtlMailboxInitialize(
    _In_ ULONG PoolSize
    )
{
    NTSTATUS Status = STATUS_INSUFFICIENT_RESOURCES;

    PAGED_CODE();

    TracePool = DsAllocatePoolWithTag( NonPagedPool, PoolSize, 'pTsD' );

    if ( TracePool ) {
        RtlRingBufferInitialize( &TraceRing, TracePool, PoolSize );
        Status = STATUS_SUCCESS;
    }

    if (NT_SUCCESS( Status )) {
        Status = RtlMailboxStartThread();

        if  (!NT_SUCCESS( Status )) {
            RtlRingBufferDestroy( &TraceRing );
            DsFreePoolWithTag( TracePool, 'pTsD' );
        }
    }

    return Status;
}

VOID
RtlMailboxDestroy(
    )
{
    PAGED_CODE();

    RtlMailboxStopThread();

    RtlRingBufferDestroy( &TraceRing );
    DsFreePoolWithTag( TracePool, 'pTsD' );
}

NTSTATUS
RtlPostMailboxTrace(
    _In_ ULONG Level,
    _In_ ULONG Area,
    __drv_formatString( printf ) _In_ PCSTR TraceMessage,
    ...
    )
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    CHAR TraceBuffer[MAILBOX_BUFFER_SIZE];
    va_list ArgumentList;
    MAILBOX_HEADER Mailbox;
    SIZE_T TraceLenght = 0;
    SIZE_T BytesRead;

    if (TraceMessage) {
        va_start( ArgumentList, TraceMessage );

        Status = RtlStringCbVPrintfA( TraceBuffer,
                                      sizeof( TraceBuffer ),
                                      TraceMessage,
                                      ArgumentList );
        if (NT_SUCCESS( Status )) {
            Status = RtlStringCbLengthA( TraceBuffer,
                                         MAILBOX_BUFFER_SIZE,
                                         &TraceLenght );
        } else {
            DbgPrint( "RtlStringCbVPrintfA failed 0x%x\n", Status );
        }

        va_end( ArgumentList );
    }

    if (!NT_SUCCESS( Status )) {
        NT_ASSERTMSG( "RtlPostMailboxTrace failed\n", FALSE );
        return Status;
    }

    Mailbox.Type = MailboxTrace;
    Mailbox.Size = TraceLenght;
    Mailbox.Trace.Level = Level;
    Mailbox.Trace.Area = Area;

    Status = RtlRingBufferWrite( &TraceRing, 
                                 (PCHAR)&Mailbox,
                                 sizeof( MAILBOX_HEADER ) );

    if (NT_SUCCESS( Status )) {
        Status = RtlRingBufferWrite( &TraceRing, TraceBuffer, TraceLenght );

        if (NT_SUCCESS( Status )) {
            KeReleaseSemaphore( &MailboxQueueSemaphore,
                                IO_NO_INCREMENT,
                                1,
                                FALSE );
        } else {
            RtlRingBufferRead( &TraceRing,
                               (PCHAR)&Mailbox,
                               sizeof( MAILBOX_HEADER ),
                               &BytesRead );
        }
    }

    return Status;
}

NTSTATUS
RtlPostMailboxNotification(
    _In_ PVOID Notification,
    _In_ SIZE_T Length
    )
{
    NTSTATUS Status;
    MAILBOX_HEADER Mailbox;
    SIZE_T BytesRead;

    Mailbox.Type = MailboxNotification;
    Mailbox.Size = Length;

    Mailbox.Notification.Reserved = 0;

    Status = RtlRingBufferWrite( &TraceRing, 
                                 (PCHAR)&Mailbox,
                                 sizeof( MAILBOX_HEADER ) );

    if (NT_SUCCESS( Status )) {
        Status = RtlRingBufferWrite( &TraceRing, Notification, Length );

        if (NT_SUCCESS( Status )) {
            KeReleaseSemaphore( &MailboxQueueSemaphore,
                                IO_NO_INCREMENT,
                                1,
                                FALSE );
        } else {
            RtlRingBufferRead( &TraceRing,
                               (PCHAR)&Mailbox,
                               sizeof( MAILBOX_HEADER ),
                               &BytesRead );
        }
    }

    return Status;
}

NTSTATUS
RtlRetrieveMailbox(
    _Inout_ PMAILBOX_HEADER Mailbox,
    _Out_writes_bytes_( DataSize ) PCHAR Data,
    _In_ SIZE_T DataSize
    )
{
    NTSTATUS Status;
    SIZE_T BytesRead;
    BOOLEAN Restore = TRUE;

    Status = RtlRingBufferRead( &TraceRing, 
                                (PCHAR)&Mailbox,
                                sizeof( MAILBOX_HEADER ),
                                &BytesRead );

    if (NT_SUCCESS( Status )) {

        if (DataSize >= Mailbox->Size) {
            Status = RtlRingBufferRead( &TraceRing, 
                                        (PCHAR)&Data,
                                        Mailbox->Size,
                                        &BytesRead );
            if (NT_SUCCESS( Status )) {
                Restore = FALSE;
            }
        }

        if (Restore) {
            RtlRingBufferWrite( &TraceRing,
                                (PCHAR)&Mailbox,
                                sizeof( MAILBOX_HEADER ) );
        }
    }

    return Status;
}