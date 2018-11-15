#ifndef _CHANNEL_
#define _CHANNEL_

#include <ntifs.h>
#include "wdk7.h"

#pragma warning(disable:4201)   // nameless struct/union

#define DS_MAX_BUCKETS    (16)
#define NO_BUCKET_FOUND   ((ULONG)-1)

typedef enum _DS_MESSAGE_TYPE {
    TraceMessage = 1,
    NotificationMessage,
    TerminateMessage,
    ErrorMessage,
    MaxMessage,
} DS_MESSAGE_TYPE;

typedef enum _DS_TERMINATE_REASON {
    TeardownReason = 1,
    NoOpenDeviceReason,
    UnloadDriverReason,
    InitErrorReason,
    MaxReason,
} DS_TERMINATE_REASON, *PDS_TERMINATE_REASON;

typedef UINT32 DS_MESSAGE_FLAGS;
    #define DS_MESSAGE_ASYNCHRONOUS     0x00000001

typedef struct _DS_MESSAGE_HEADER {
    UINT64 UniqueId;
    DS_MESSAGE_FLAGS ControlFlags;
    DS_MESSAGE_TYPE MessageType;
} DS_MESSAGE_HEADER;

typedef enum _DS_NOTIFICATION_TYPE {
    TimerAbuse = 1,
    TimerFalsePositive = 2,
    RestrictedAccess,
    MaxType,
} DS_NOTIFICATION_TYPE, *PDS_NOTIFICATION_TYPE;

typedef enum _DS_NOTIFICATION_ACTION {
    ActionAllow = 1,
    ActionDeny,
    MaxAction,
} DS_NOTIFICATION_ACTION, *PDS_NOTIFICATION_ACTION;

typedef struct _DS_NOTIFICATION_MESSAGE {
    DS_MESSAGE_HEADER;
    UINT64 ProcessId;
    UINT64 ThreadId;
    DS_NOTIFICATION_TYPE Type;
    DS_NOTIFICATION_ACTION Action;
} DS_NOTIFICATION_MESSAGE, *PDS_NOTIFICATION_MESSAGE;

typedef struct _DS_TERMINATE_MESSAGE {
    DS_MESSAGE_HEADER;
    DS_TERMINATE_REASON Reason;
} DS_TERMINATE_MESSAGE, *PDS_TERMINATE_MESSAGE;

typedef enum _DS_EVENT_TYPE {
    LowEvent = 0,
    HighEvent,
    MaxEvent,
} DS_EVENT_TYPE;

typedef struct _DS_BUCKET_SERVER {
    PKEVENT PairEvent[MaxEvent];
} DS_BUCKET_SERVER, *PDS_BUCKET_SERVER;

typedef struct _DS_BUCKET_CLIENT {
    HANDLE EventHandle[MaxEvent];
    UCHAR Data[240];
} DS_BUCKET_CLIENT, *PDS_BUCKET_CLIENT;

typedef struct _DS_CHANNEL {
    ULONG ChannelId;
    PMDL Mdl;
    PVOID UserVa;
    ULONG UserSize;
    LONG BucketsBitMap;
    USHORT BucketCount;
    DS_BUCKET_SERVER ServerBuckets[DS_MAX_BUCKETS];
    DS_BUCKET_CLIENT Buckets[DS_MAX_BUCKETS];
} DS_CHANNEL, *PDS_CHANNEL;

NTSTATUS
DsInitializeChannel(
    _In_ USHORT BucketCount,
    _Out_ PDS_CHANNEL *NewChannel
    );

VOID
DsDestroyChannel(
    _Inout_ PDS_CHANNEL Channel,
    _In_ DS_TERMINATE_REASON Reason
    );

NTSTATUS
DsAcquireChannelBucket(
    _Inout_ PDS_CHANNEL Channel,
    _Out_ PULONG BucketIndex
    );

VOID
DsReleaseChannelBucket(
    _Inout_ PDS_CHANNEL Channel,
    _In_ ULONG BucketIndex
    );

NTSTATUS
DsSendNotificationMessage(
    _In_ PDS_CHANNEL Channel,
    _In_ UINT64 ProcessId,
    _In_ UINT64 ThreadId,
    _In_ DS_NOTIFICATION_TYPE Type,
    _Out_ PDS_NOTIFICATION_ACTION Action
    );

#endif