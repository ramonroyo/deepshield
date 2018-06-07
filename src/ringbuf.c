/*++

Copyright (c) ByteHeed. All Rights Reserved.

Module Name:

    ringbuf.c

Abstract:

    This file implements the RingBuffer.

Environment:

--*/

#include <ntifs.h>
#include "ringbuf.h"

#define RB_AVAILABLE_BYTES( h, t, b ) \
    ((t) >= (h)) ? ((b) - ((t) - (h))) : ((h) - (t))

VOID
RtlRingBufferInitialize(
    _In_ PRING_BUFFER RingBuffer,
    _In_reads_bytes_(BufferSize) PUCHAR Buffer,
    _In_ SIZE_T BufferSize
    )
{
    RingBuffer->Size = BufferSize;
    RingBuffer->Base = Buffer;
    RingBuffer->End  = Buffer + BufferSize;
    RingBuffer->Head = Buffer;
    RingBuffer->Tail = Buffer;

    KeInitializeSpinLock( &RingBuffer->Lock );
}

VOID
RtlRingBufferDestroy(
    _In_ PRING_BUFFER RingBuffer
    )
{
    UNREFERENCED_PARAMETER( RingBuffer );
}

VOID
RtlRingBufferGetAvailBytes(
    _In_ PRING_BUFFER RingBuffer,
    _Out_ PSIZE_T WriteSize,
    _Out_ PSIZE_T ReadSize
    )
{
    PUCHAR Head;
    PUCHAR Tail;

    Head = RingBuffer->Head;
    Tail = RingBuffer->Tail;

    *WriteSize = RB_AVAILABLE_BYTES( Head, Tail, RingBuffer->Size );
    *ReadSize = RingBuffer->Size - *WriteSize;
}

NTSTATUS
RtlRingBufferWrite(
    _In_ PRING_BUFFER RingBuffer,
    _In_reads_bytes_(DataSize) PCHAR Data,
    _In_ SIZE_T DataSize
    )
{
    SIZE_T AvailToWrite;
    SIZE_T AvailToRead;
    SIZE_T BytesToCopy;
    SIZE_T SpaceToEnd;
    KIRQL OldIrql;

    NT_ASSERT( Data && (0 != DataSize) );

    if ( RingBuffer->Tail >= RingBuffer->End) {
        return STATUS_INTERNAL_ERROR;
    }

    KeAcquireSpinLock( &RingBuffer->Lock, &OldIrql );

    RtlRingBufferGetAvailBytes( RingBuffer, &AvailToWrite, &AvailToRead );

    //
    // If there is not enough space then make room to fit in all the data.
    //
    if (AvailToWrite <= DataSize) {
        //
        //  TODO: make room for write -> RingBufferDelete();
        //
        BytesToCopy = AvailToWrite;
    }
    else {
        BytesToCopy = DataSize;
    }

    if (BytesToCopy) {

        if (RingBuffer->Tail + BytesToCopy > RingBuffer->End)  {
            //
            // The data being written will wrap around the end of the buffer.
            //

            SpaceToEnd = RingBuffer->End - RingBuffer->Tail;
            RtlCopyMemory( RingBuffer->Tail, Data, SpaceToEnd );

            Data += SpaceToEnd;
            BytesToCopy -= SpaceToEnd;
            RtlCopyMemory( RingBuffer->Base, Data, BytesToCopy );

            //
            //  Advance the tail pointer.
            //
            RingBuffer->Tail = RingBuffer->Base + BytesToCopy;
        }
        else {
            //
            // Data does NOT wrap around the end of the buffer. Just copy it
            // over in a single step.
            //
            RtlCopyMemory( RingBuffer->Tail, Data, BytesToCopy );

            //
            //  Advance the tail pointer.
            //
            RingBuffer->Tail += BytesToCopy;
            if ( RingBuffer->Tail == RingBuffer->End) {

                //
                //  We have exactly reached the end of the buffer. The next
                //  write should wrap around and start from the beginning.
                //
                RingBuffer->Tail = RingBuffer->Base;
            }
        }

        NT_ASSERT( RingBuffer->Tail < RingBuffer->End );
    }

    KeReleaseSpinLock( &RingBuffer->Lock, OldIrql );

    return STATUS_SUCCESS;
}

NTSTATUS
RtlRingBufferRead(
    _In_ PRING_BUFFER RingBuffer,
    _Out_writes_bytes_to_(DataSize, *BytesCopied) PCHAR Data,
    _In_ SIZE_T DataSize,
    _Out_ PSIZE_T BytesCopied
    )
{
    SIZE_T AvailToWrite;
    SIZE_T AvailToRead;
    SIZE_T DataToEnd;
    KIRQL OldIrql;

    NT_ASSERT( Data && (DataSize != 0) );

    *BytesCopied = 0;

    if ( RingBuffer->Head >= RingBuffer->End) {
        return STATUS_INTERNAL_ERROR;
    }

    KeAcquireSpinLock( &RingBuffer->Lock, &OldIrql );

    RtlRingBufferGetAvailBytes( RingBuffer, &AvailToWrite, &AvailToRead );

    if (AvailToRead == 0) {
        KeReleaseSpinLock( &RingBuffer->Lock, OldIrql );
        return STATUS_SUCCESS;
    }

    if (DataSize > AvailToRead ) {
        DataSize = AvailToRead;
    }

    *BytesCopied = DataSize;

    if (RingBuffer->Head + DataSize > RingBuffer->End) {

        //
        //  The data requested is wrapped around the end of the buffer.
        //

        DataToEnd = RingBuffer->End - RingBuffer->Head;
        RtlCopyMemory( Data, RingBuffer->Head, DataToEnd );
        
        Data += DataToEnd;
        DataSize -= DataToEnd;
        RtlCopyMemory( Data, RingBuffer->Base, DataSize );

        //
        //  Advance the head pointer.
        //
        RingBuffer->Head = RingBuffer->Base + DataSize;
    }
    else {
        //
        //  The data in the buffer is NOT wrapped around the end of the buffer.
        //
        RtlCopyMemory( Data, RingBuffer->Head, DataSize );

        //
        // Advance the head pointer.
        //
        RingBuffer->Head += DataSize;
        if ( RingBuffer->Head == RingBuffer->End) {

            //
            // We have exactly reached the end of the buffer. The next
            // read should wrap around and start from the beginning.
            //
            RingBuffer->Head = RingBuffer->Base;
        }
    }

    NT_ASSERT( RingBuffer->Head < RingBuffer->End) ;

    KeReleaseSpinLock( &RingBuffer->Lock, OldIrql );

    return STATUS_SUCCESS;
}