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
#pragma alloc_text(PAGE, RtlMailboxStartWorker)
#pragma alloc_text(PAGE, RtlMailboxStopWorker)
#pragma alloc_text(PAGE, RtlMailboxInitialize)
#pragma alloc_text(PAGE, RtlMailboxDestroy)
#pragma alloc_text(PAGE, RtlMailboxWorkerThread)
#endif

KSTART_ROUTINE RtlMailboxWorkerThread;

MAILBOX gSecureMailbox;

VOID
RtlMailboxWorkerThread(
    _In_ PVOID Context
);

NTSTATUS
RtlMailboxStartWorker(
    _Inout_ PMAILBOX Mailbox
    )
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Oa = { 0 };
    HANDLE ThreadHandle;

    PAGED_CODE();

    KeInitializeSemaphore( &Mailbox->QueueSemaphore, 0, MAXLONG );
    KeInitializeEvent( &Mailbox->ShutdownEvent, NotificationEvent, FALSE );

    InitializeObjectAttributes( &Oa, NULL, OBJ_KERNEL_HANDLE, NULL, NULL );
    Status = PsCreateSystemThread( &ThreadHandle,
                                   THREAD_ALL_ACCESS, 
                                   &Oa,
                                   NULL,
                                   NULL,
                                   Mailbox->WorkerThread,
                                   (PVOID)Mailbox );

    if (NT_SUCCESS( Status )) {
        Status = ObReferenceObjectByHandle( ThreadHandle,
                                            THREAD_ALL_ACCESS,
                                            *PsThreadType,
                                            KernelMode, 
                                            (PVOID*)&Mailbox->Thread,
                                            NULL );
        if (NT_SUCCESS( Status ) ) {
            ZwClose( ThreadHandle );

        } else {
            RtlMailboxStopWorker( Mailbox );
            NT_VERIFYMSG( "ObReferenceObjectByHandle can't fail with a valid kernel handle",
                          NT_SUCCESS( Status ) );
        }
    }

    return Status;
}

VOID 
RtlMailboxStopWorker(
    _Inout_ PMAILBOX Mailbox
    )
{
    PAGED_CODE();

    KeSetEvent( &Mailbox->ShutdownEvent, IO_NO_INCREMENT, FALSE );

    if (Mailbox->Thread) {
        KeWaitForSingleObject( Mailbox->Thread,
                               Executive,
                               KernelMode,
                               FALSE,
                               NULL );

        ObDereferenceObject( Mailbox->Thread );
        Mailbox->Thread = NULL;
    }
}

NTSTATUS
RtlDispatchMailboxData(
    _Inout_ PMAILBOX Mailbox
    )
{
    NTSTATUS Status;
    PDS_NOTIFICATION_MESSAGE Message;
    PDS_PROCESS_ENTRY ProcessEntry;
    MAILBOX_RECORD MailboxEntry;

    Status = RtlRetrieveMailboxData( Mailbox, &MailboxEntry );

    if (!NT_SUCCESS( Status )) {
        NT_VERIFYMSG( "RtlRetrieveMailboxData failed with a fixed data size",
                      NT_SUCCESS( Status ) );
        return Status;
    }

    if (MailboxEntry.Header.Type == MailboxTrace) {
        //
        //  The end result of a mailbox trace is just to generate a
        //  trace event for debugging purposes.
        //

        switch( MailboxEntry.Header.Trace.Area ) {
            case TRACE_IOA_ROOT:

                switch( MailboxEntry.Header.Trace.Level ) {
                    case TRACE_LEVEL_VERBOSE:
                        TraceEvents( TRACE_LEVEL_VERBOSE, TRACE_IOA,
                                     "%s\n",
                                     (PCHAR)MailboxEntry.Data );
                        break;

                    case TRACE_LEVEL_INFORMATION:
                        TraceEvents( TRACE_LEVEL_INFORMATION, TRACE_IOA,
                                     "%s\n",
                                     (PCHAR) MailboxEntry.Data );
                        break;

                    default:
                        TraceEvents( TRACE_LEVEL_WARNING, TRACE_IOA,
                                     "%s\n",
                                     (PCHAR) MailboxEntry.Data );
                        break;
                }
                break;
            
            case TRACE_MSR_ROOT:

                switch ( MailboxEntry.Header.Trace.Level ) {
                    case TRACE_LEVEL_VERBOSE:
                        TraceEvents( TRACE_LEVEL_VERBOSE, TRACE_MSR,
                                     "%s\n",
                                     (PCHAR) MailboxEntry.Data );
                        break;

                    case TRACE_LEVEL_INFORMATION:
                        TraceEvents( TRACE_LEVEL_INFORMATION, TRACE_MSR,
                                     "%s\n",
                                     (PCHAR) MailboxEntry.Data );
                        break;

                    default:
                        TraceEvents( TRACE_LEVEL_WARNING, TRACE_MSR,
                                     "%s\n",
                                     (PCHAR) MailboxEntry.Data );
                        break;
                }
                break;
        }
    }
    else if (MailboxEntry.Header.Type == MailboxNotification) {
        Message = (PDS_NOTIFICATION_MESSAGE) MailboxEntry.Data;

        if (TimerFalsePositive == Message->Type) {

            ProcessEntry = PmLookupProcessEntryById( 
                (HANDLE)((PLARGE_INTEGER)&Message->ProcessId)->LowPart );

            if (ProcessEntry) {
                Status = PmExcludeThread( &ProcessEntry->ThreadList, 
                                          Message->ThreadId,
                                          TRUST_REASON_FALSE_POSITIVE );

                if (!NT_SUCCESS( Status )) {
                    TraceEvents( TRACE_LEVEL_WARNING, TRACE_MAILBOX,
                                 "PmExcludeThread failed (ProcessId: %p ThreadId: %016I64x)\n",
                                 ProcessEntry->ProcessId,
                                 Message->ThreadId );
                }
            }
        }
        else {
            NT_ASSERT( TimerAbuse == Message->Type );

            Status = DsSendNotificationMessage( gChannel,
                                                Message->ProcessId,
                                                Message->ThreadId,
                                                Message->Type,
                                                &Message->Action );
        }
    } else {
        NT_ASSERT( FALSE );
        TraceEvents( TRACE_LEVEL_ERROR, TRACE_MAILBOX,
                     "Unrecognized mailbox data type (Type: %d)\n",
                     MailboxEntry.Header.Type );
    }

    return Status;
}

VOID
RtlMailboxWorkerThread(
    _In_ PVOID Context
    )
{
    NTSTATUS Status;
    PVOID WaitObjects[2];
    PMAILBOX Mailbox = (PMAILBOX) Context;
    LARGE_INTEGER DueTime;

    PAGED_CODE();

    WaitObjects[0] = &Mailbox->ShutdownEvent;
    WaitObjects[1] = &Mailbox->QueueSemaphore;

    DueTime.QuadPart = REL_TIMEOUT_IN_MS( 360 );

    KeSetPriorityThread( KeGetCurrentThread(), LOW_REALTIME_PRIORITY );

    for (;;) {
        Status = KeWaitForMultipleObjects( ARRAYSIZE( WaitObjects ),
                                           WaitObjects,
                                           WaitAny,
                                           Executive,
                                           KernelMode,
                                           FALSE,
                                           &DueTime,
                                           NULL );

        if (Status == STATUS_WAIT_0) {
            break;
        }
#ifdef LET_SWAP_CONTEXT
        else if (Status == STATUS_WAIT_1) {
            Status = RtlDispatchMailboxData( Mailbox );
        }
#else
        else if (Status == STATUS_TIMEOUT) {

            while (!RtlIsMailboxEmpty( Mailbox )) {
                Status = RtlDispatchMailboxData( Mailbox );
            }
        }
#endif
        else { 
            NT_VERIFYMSG( "Unexpected wait result in Mailbox thread", FALSE );
        }
    }

    TraceEvents( TRACE_LEVEL_INFORMATION, TRACE_MAILBOX,
                 "Mailbox worker thread is about to terminate\n" );
    PsTerminateSystemThread( STATUS_SUCCESS );
}

NTSTATUS
RtlMailboxInitialize(
    _Inout_ PMAILBOX Mailbox,
    _In_ ULONG PoolSize
    )
{
    NTSTATUS Status = STATUS_INSUFFICIENT_RESOURCES;
    PAGED_CODE();

    RtlZeroMemory( Mailbox, sizeof( MAILBOX ) );

    Mailbox->WorkerThread = RtlMailboxWorkerThread;
    Mailbox->PoolBuffer = DsAllocatePoolWithTag( NonPagedPool, PoolSize, 'pMsD' );

    if (Mailbox->PoolBuffer) {
        RtlRingBufferInitialize( &Mailbox->RingBuffer, 
                                 Mailbox->PoolBuffer,
                                 PoolSize );

        Status = RtlMailboxStartWorker( Mailbox );

        if (!NT_SUCCESS( Status )) {
            RtlMailboxDestroy( Mailbox );
        }
    }

    return Status;
}

VOID
RtlMailboxDestroy(
    _Inout_ PMAILBOX Mailbox
    )
{
    PAGED_CODE();

    RtlMailboxStopWorker( Mailbox );
    RtlRingBufferDestroy( &Mailbox->RingBuffer );

    DsFreePoolWithTag( Mailbox->PoolBuffer, 'pMsD' );
}

BOOLEAN
RtlIsMailboxEmpty(
    _In_ PMAILBOX Mailbox
    )
{
    SIZE_T AvailToWrite;
    SIZE_T AvailToRead;

    RtlRingBufferGetAvailBytes( &Mailbox->RingBuffer, 
                                &AvailToWrite,
                                &AvailToRead );

    //
    //  A record is present only if is contains a header and a body.
    //
    return (AvailToRead < sizeof( MAILBOX_RECORD ));
}

NTSTATUS
RtlPostMailboxTrace(
    _Inout_ PMAILBOX Mailbox,
    _In_ ULONG Level,
    _In_ ULONG Area,
    __drv_formatString( printf ) _In_ PCSTR TraceMessage,
    ...
    )
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    MAILBOX_RECORD Entry;
    va_list ArgumentList;
    SIZE_T TraceLenght = 0;

    if (TraceMessage) {
        va_start( ArgumentList, TraceMessage );

        Status = RtlStringCbVPrintfA( (PCHAR)Entry.Data,
                                      sizeof( Entry.Data ),
                                      TraceMessage,
                                      ArgumentList );
        if (NT_SUCCESS( Status )) {
            Status = RtlStringCbLengthA( (PCHAR)Entry.Data,
                                         MAILBOX_BUFFER_SIZE,
                                         (size_t*)&TraceLenght );
        } else {
            DbgPrint( "RtlStringCbVPrintfA failed 0x%x\n", Status );
        }

        va_end( ArgumentList );
    }

    if (!NT_SUCCESS( Status )) {

        NT_ASSERT( FALSE );
        return Status;
    }

    Entry.Header.Type = MailboxTrace;
    Entry.Header.Size = TraceLenght;
    Entry.Header.Trace.Level = Level;
    Entry.Header.Trace.Area = Area;

    Status = RtlRingBufferWrite( &Mailbox->RingBuffer,
                                 (PCHAR)&Entry.Header,
                                 sizeof( MAILBOX_RECORD ) );

    if (NT_SUCCESS( Status )) {
#ifdef LET_SWAP_CONTEXT
        KeReleaseSemaphore( &Mailbox->QueueSemaphore,
                            IO_NO_INCREMENT,
                            1,
                            FALSE );
#endif
    }

    return Status;
}

NTSTATUS
RtlPostMailboxNotification(
    _Inout_ PMAILBOX Mailbox,
    _In_ PVOID Notification,
    _In_ SIZE_T Length
    )
{
    NTSTATUS Status;
    MAILBOX_RECORD Entry;

    Entry.Header.Type = MailboxNotification;
    Entry.Header.Size = Length;
    Entry.Header.Notification.Reserved = 0;

    NT_ASSERT( Length <= sizeof( Entry.Data ) );

    if (Length > sizeof( Entry.Data ) ) {
        DbgPrint( "Data was too large to fit into the mailbox notification\n" );
        return STATUS_BUFFER_OVERFLOW;
    }

    RtlCopyMemory( Entry.Data, Notification, Length );

    Status = RtlRingBufferWrite( &Mailbox->RingBuffer,
                                 (PCHAR)&Entry.Header,
                                 sizeof( MAILBOX_RECORD ) );

    if (NT_SUCCESS( Status )) {
#ifdef LET_SWAP_CONTEXT
        KeReleaseSemaphore( &Mailbox->QueueSemaphore,
                            IO_NO_INCREMENT,
                            1,
                            FALSE );
#endif
    }

    return Status;
}

NTSTATUS
RtlRetrieveMailboxData(
    _Inout_ PMAILBOX Mailbox,
    _Inout_ PMAILBOX_RECORD MailboxEntry
    )
{
    NTSTATUS Status;
    SIZE_T BytesRead;

    MailboxEntry->Header.Type = MailboxEmpty;

    Status = RtlRingBufferRead( &Mailbox->RingBuffer,
                                (PCHAR) MailboxEntry,
                                sizeof( MAILBOX_RECORD ),
                                &BytesRead );

    NT_ASSERT( NT_SUCCESS( Status ) );
    return Status;
}