/*++

Copyright (c) ByteHeed. All Rights Reserved.

Module Name:

    channel.c

Abstract:

    This file implements the channel to communicate messages with
    user mode.

Environment:

--*/

#include "dsdef.h"

#if defined(WPP_EVENT_TRACING)
#include "channel.tmh"
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, DsInitializeChannel)
#pragma alloc_text(PAGE, DsDestroyChannel)
#pragma alloc_text(PAGE, DsAcquireChannelBucket)
#pragma alloc_text(PAGE, DsReleaseChannelBucket)
#pragma alloc_text(PAGE, DsSendNotificationMessage)
#endif

static volatile LONG64 UniqueId;

NTSTATUS
DsInitializeBucketEvents(
    _Inout_ PDS_BUCKET_CLIENT Bucket,
    _Inout_ PDS_BUCKET_SERVER ServerBucket
    )
{
    NTSTATUS Status;
    UniqueId = 0;

    //
    //  IoCreateSynchronizationEvent creates a kernel handle so it is required
    //  to create it manually to place it in the context of the helper process.
    //
    Status = ZwCreateEvent( &Bucket->EventHandle[LowEvent],
                            EVENT_ALL_ACCESS,
                            NULL,
                            SynchronizationEvent,
                            FALSE );

    if (!NT_SUCCESS( Status )) {
        return Status;
    }

    Status = ObReferenceObjectByHandle( 
                            Bucket->EventHandle[LowEvent],
                            EVENT_ALL_ACCESS,
                            *ExEventObjectType,
                            UserMode,
                            &ServerBucket->PairEvent[LowEvent],
                            NULL );

    if (!NT_SUCCESS( Status )) {
        ZwClose( Bucket->EventHandle[LowEvent] );
        return Status;
    }

    Status = ZwCreateEvent( &Bucket->EventHandle[HighEvent],
                            EVENT_ALL_ACCESS,
                            NULL,
                            SynchronizationEvent,
                            FALSE );

    if (!NT_SUCCESS( Status )) {
        ObDereferenceObject( ServerBucket->PairEvent[LowEvent] );
        ZwClose( Bucket->EventHandle[LowEvent] );

        return Status;
    }

    Status = ObReferenceObjectByHandle( 
                            Bucket->EventHandle[HighEvent],
                            EVENT_ALL_ACCESS,
                            *ExEventObjectType,
                            UserMode,
                            &ServerBucket->PairEvent[HighEvent],
                            NULL );
    
    if (!NT_SUCCESS( Status )) {
        ZwClose( Bucket->EventHandle[HighEvent] );

        ObDereferenceObject( ServerBucket->PairEvent[LowEvent] );
        ZwClose( Bucket->EventHandle[LowEvent] );

        return Status;
    }

    return Status;
}

VOID
DsCleanupBucketEvents(
    _Inout_ PDS_BUCKET_CLIENT Bucket,
    _Inout_ PDS_BUCKET_SERVER ServerBucket
    )
{
    //
    //  Explicitly call NtClose() instead of ZwClose() so that PreviousMode()
    //  will be User. This prevents accidental closing of a kernel handle and
    //  also will not bugcheck the system if the handle value is no longer
    //  valid.
    //

    if (Bucket->EventHandle[HighEvent]) {
        if (ServerBucket->PairEvent[HighEvent]) {
            ObDereferenceObject( ServerBucket->PairEvent[HighEvent] );
        }

        NtClose( Bucket->EventHandle[HighEvent] );
    }

    if (Bucket->EventHandle[LowEvent]) {
        if (ServerBucket->PairEvent[LowEvent]) {
            ObDereferenceObject( ServerBucket->PairEvent[LowEvent] );
        }

        NtClose( Bucket->EventHandle[LowEvent] );
    }
}

NTSTATUS
DsInitializeChannel(
    _In_ USHORT BucketCount,
    _Out_ PDS_CHANNEL *NewChannel
    )
/*++

Routine Description:

   This routine allocates the channel memory from non paged pool and
   map it into user space.Then initializes the collection of events
   used to synchronize the channels' buckets.

Arguments:

    BucketCount - number of buckets to allocate for this channel.

    NewChannel - returns a pointer to the newly creted channel.

Return Value:

   NT status code

--*/
{
    NTSTATUS Status = STATUS_INSUFFICIENT_RESOURCES;
    PDS_CHANNEL Channel = NULL;
    LARGE_INTEGER TickCount;
    ULONG MappingPriority = NormalPagePriority;
    ULONG BucketsSize;
    ULONG Idx;

    PAGED_CODE();

    *NewChannel = NULL;

    BucketsSize = sizeof( DS_BUCKET_CLIENT ) * BucketCount;
    Channel = ExAllocatePoolWithTag( NonPagedPool, 
                                     sizeof( DS_CHANNEL ),
                                     'bReS' );

    if (NULL == Channel) {
        goto RoutineExit;
    }

    RtlZeroMemory( Channel, sizeof( DS_CHANNEL ) );

    KeQueryTickCount( &TickCount );
    Channel->ChannelId = TickCount.LowPart;

    Channel->BucketsBitMap = (1Ui32 << DS_MAX_BUCKETS) - 1;
    Channel->BucketCount = BucketCount;

    if (RtlIsNtDdiVersionAvailable( NTDDI_WIN8 )) {
        MappingPriority |= MdlMappingNoExecute;
    }

    Channel->Mdl = IoAllocateMdl( &Channel->Buckets[0],
                                  BucketsSize,
                                  FALSE,
                                  FALSE,
                                  NULL );

    if (NULL == Channel->Mdl) {
        goto RoutineExit;
    }

    MmBuildMdlForNonPagedPool( Channel->Mdl );

    __try {
        //
        //  If the specified pages cannot be mapped the routine raises
        //  an exception.
        //
        Channel->UserVa = 
            MmMapLockedPagesSpecifyCache( 
                                    Channel->Mdl,
                                    UserMode,
                                    MmNonCached,
                                    NULL,
                                    FALSE,
                                    MappingPriority );
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Channel->UserVa = NULL;
    }

    if (Channel->UserVa) {
        Channel->UserSize = BucketsSize;

        NT_ASSERT( Channel->UserSize == MmGetMdlByteCount( Channel->Mdl ) );

        for (Idx = 0; Idx < BucketCount; Idx++) {
            Status = DsInitializeBucketEvents( &Channel->Buckets[Idx],
                                               &Channel->ServerBuckets[Idx] );

            if (!NT_SUCCESS( Status )) {
                break;
            }
        }

        if (NT_SUCCESS( Status )) {
            *NewChannel = Channel;

            TraceEvents( TRACE_LEVEL_INFORMATION, TRACE_CHANNEL,
                         "Channel initialized\tChannel: %p\n",
                         Channel );
        }
    }
    else {
        Status = STATUS_NONE_MAPPED;
    }

RoutineExit:

    if (!NT_SUCCESS( Status )) {
        if (Channel) {
            DsDestroyChannel( Channel, InitErrorReason );
        }

        TraceEvents( TRACE_LEVEL_ERROR, TRACE_CHANNEL,
                     "Error initializing channel\tStatus: %!STATUS!\n",
                     Status );
    }

    return Status;
}

VOID
DsDestroyChannel(
    _Inout_ PDS_CHANNEL Channel,
    _In_ DS_TERMINATE_REASON Reason
    )
{
    ULONG Idx;
    DS_TERMINATE_MESSAGE Message;
    LARGE_INTEGER DueTime;
    PKEVENT DieEvent;
    PKEVENT AckEvent;

    PAGED_CODE();

    TraceEvents( TRACE_LEVEL_INFORMATION,
                 TRACE_CHANNEL,
                 "Channel destroyed\n" );

    DueTime.QuadPart = REL_TIMEOUT_IN_MS( 3000 );
    //
    //  It is time to signal termination and then wait for the helper's
    //  acknowledgement.
    //

    Message.UniqueId = 0;
    Message.MessageType = TerminateMessage;
    Message.ControlFlags = 0;

    Message.Reason = Reason;

    for (Idx = 0; Idx < Channel->BucketCount; Idx++) {

        RtlCopyMemory( Channel->Buckets[Idx].Data,
                       &Message,
                       sizeof( DS_TERMINATE_MESSAGE ) );

        DieEvent = Channel->ServerBuckets[Idx].PairEvent[HighEvent];
        KeSetEvent( DieEvent, IO_NO_INCREMENT, FALSE );

        AckEvent = Channel->ServerBuckets[Idx].PairEvent[LowEvent];
        KeWaitForSingleObject( AckEvent,
                               Executive,
                               KernelMode,
                               FALSE,
                               &DueTime );
    }

    for (Idx = 0; Idx < Channel->BucketCount; Idx++) {
        DsCleanupBucketEvents( &Channel->Buckets[Idx],
                               &Channel->ServerBuckets[Idx] );
    }

    if (Channel->Mdl) {
        if (Channel->UserVa) {
            MmUnmapLockedPages( Channel->UserVa, Channel->Mdl );
        }

        IoFreeMdl( Channel->Mdl );
    }

    ExFreePoolWithTag( Channel, 'bReS' );
}

NTSTATUS
DsAcquireChannelBucket(
    _Inout_ PDS_CHANNEL Channel,
    _Out_ PULONG BucketIndex
    )
/*++

Routine Description:

    This routine searches the channel bucket bitmap to get a free bucket
    index and clears that bit in the bitmap if any is found. 

Arguments:

    Channel - Supplies a pointer to the channel.

    BucketIndex - Receives the index (zero based) of a free bucket indicating
        the position. If such a bucket cannot be located a -1 is received.

Return Value:

    NTSTATUS - Status code for the operation.
--*/
{
    CCHAR FreeIndex;

    PAGED_CODE();

    *BucketIndex = NO_BUCKET_FOUND;
    
    for (;;) {

        FreeIndex = RtlFindMostSignificantBit( Channel->BucketsBitMap );
        if (FreeIndex != -1) {

            if (FALSE == InterlockedBitTestAndReset( &Channel->BucketsBitMap,
                                                     FreeIndex )) {
                continue;
            }

            *BucketIndex = (ULONG)FreeIndex;
            return STATUS_SUCCESS;
        }

        return STATUS_INSUFFICIENT_RESOURCES;
    }
}

VOID
DsReleaseChannelBucket(
    _Inout_ PDS_CHANNEL Channel,
    _In_ ULONG BucketIndex
    )
{
    BOOLEAN Result;

    PAGED_CODE();
    Result = InterlockedBitTestAndSet( &Channel->BucketsBitMap,
                                       BucketIndex );
    NT_ASSERT( Result == FALSE ) ;
}

NTSTATUS
DsSendNotificationMessage(
    _In_ PDS_CHANNEL Channel,
    _In_ UINT64 ProcessId,
    _In_ UINT64 ThreadId,
    _In_ DS_NOTIFICATION_TYPE Type,
    _Out_ PDS_NOTIFICATION_ACTION Action
    )
{
    NTSTATUS Status;
    PDS_NOTIFICATION_MESSAGE Message;
    PDS_BUCKET_CLIENT Bucket;
    PKEVENT *PairEvent;
    ULONG BucketIndex;
    BOOLEAN ValidChannel;

    PAGED_CODE();

    if (!FlagOn( gStateFlags, DSH_GFL_CHANNEL_SETUP )) {
        return STATUS_NOT_SUPPORTED;
    }

    TraceEvents( TRACE_LEVEL_INFORMATION,
                 TRACE_NOTIFICATION,
                 "Sending Notification\tProcessId: %016I64x\tThreadId: %016I64x\tType: %d\n",
                 ProcessId,
                 ThreadId,
                 Type );

    ValidChannel = ExAcquireRundownProtection( &gChannelRundown );

    if ( FALSE == ValidChannel ) {
        return STATUS_DELETE_PENDING;
    }

    Status = DsAcquireChannelBucket( Channel, &BucketIndex );

    if (NT_SUCCESS( Status )) {

        Bucket = &Channel->Buckets[BucketIndex];
        PairEvent = Channel->ServerBuckets[BucketIndex].PairEvent;

        Message = (PDS_NOTIFICATION_MESSAGE)Bucket->Data;

        Message->UniqueId = InterlockedIncrement64( &UniqueId );
        Message->MessageType = NotificationMessage;
        Message->ControlFlags = 0;

        Message->ProcessId = ProcessId;
        Message->ThreadId = ThreadId;
        Message->Type = Type;

        NT_ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

        KeSetEvent( PairEvent[HighEvent], IO_NO_INCREMENT, FALSE );

        if (KeGetCurrentIrql() == PASSIVE_LEVEL) {
            KeWaitForSingleObject( PairEvent[LowEvent],
                                   Executive,
                                   KernelMode,
                                   FALSE,
                                   NULL);

            if (ARGUMENT_PRESENT( Action )) {
                *Action = Message->Action;
            }
        }
        else {

            NT_ASSERT( FALSE );
            //
            //  TODO: Queue the wait to another thread or implement
            //  reply messages to release bucket when no longer used.
            //
        }

        DsReleaseChannelBucket( Channel, BucketIndex );
    }

    ExReleaseRundownProtection( &gChannelRundown );
    return Status;
}