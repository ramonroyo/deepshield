/*++

Copyright (C) ByteHeed. All Rights Reserved

Module Name:

    ringbuf.h

--*/

#pragma once

#include <ntifs.h>
#include "wdk7.h"

#pragma warning(disable:4201)   // nameless struct/union

typedef enum _RING_BUFFER_TYPE {
    TraceBuffer = 1,
    NotificationBuffer
} RING_BUFFER_TYPE;

typedef struct _RING_BUFFER_HEADER {
    UINT64 UniqueId;
    RING_BUFFER_TYPE Type;
    UINT32 DataSize;
} RING_BUFFER_HEADER;

typedef struct _RING_BUFFER_TRACE {
    RING_BUFFER_HEADER;
    UCHAR Message[ 200 ];
} RING_BUFFER_TRACE;

typedef enum _RING_BUFFER_NOTIFICATION_TYPE {
    TscTimingAttack = 1
} RING_BUFFER_NOTIFICATION_TYPE;

typedef struct _RING_BUFFER_NOTIFICATION {
    RING_BUFFER_HEADER;
    RING_BUFFER_NOTIFICATION_TYPE NotificationType;
    ULONG64 ProcessId;
    ULONG64 ThreadId;
} RING_BUFFER_NOTIFICATION;

typedef struct _RING_BUFFER {
    SIZE_T Size;
    KSPIN_LOCK Lock;

    //
    //  A pointer to the base of the ring buffer.
    //
    PUCHAR Base;

    //
    //  A pointer to the byte beyond the end of the ring buffer.
    //
    PUCHAR End;

    //
    //  A pointer to the current read point in the ring buffer.
    //
    PUCHAR Head;

    //
    //  A pointer to the current write point in the ring buffer.
    //
    PUCHAR Tail;

} RING_BUFFER, *PRING_BUFFER;

VOID
RtlRingBufferInitialize(
    _In_ PRING_BUFFER RingBuffer,
    _In_reads_bytes_( BufferSize ) PUCHAR Buffer,
    _In_ SIZE_T BufferSize
    );

VOID
RtlRingBufferDestroy(
    _In_ PRING_BUFFER RingBuffer
    );

VOID
RtlRingBufferGetAvailBytes(
    _In_ PRING_BUFFER RingBuffer,
    _Out_ PSIZE_T WriteSize,
    _Out_ PSIZE_T ReadSize
    );

NTSTATUS
RtlRingBufferWrite(
    _In_  PRING_BUFFER RingBuffer,
    _In_reads_bytes_( DataSize ) PCHAR Data,
    _In_ SIZE_T DataSize
);

NTSTATUS
RtlRingBufferRead(
    _In_ PRING_BUFFER RingBuffer,
    _Out_writes_bytes_to_( DataSize, *BytesCopied ) PCHAR Data,
    _In_ SIZE_T DataSize,
    _Out_ PSIZE_T BytesCopied
);