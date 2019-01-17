/*++

Copyright (C) ByteHeed. All Rights Reserved

Module Name:

    process.h

--*/

#ifndef _PROCESS_H_
#define _PROCESS_H_

#include "dsdef.h"

#define DS_TAG_PROCESS_ENTRY     'ePsD'
#define DS_TAG_PROCESS_NAME      'nPsD'
#define DS_TAG_TOKEN_USER        'uTsD'

#define MAX_PROCESS_NAME        512L
#define MAX_SID_LENGTH          192L

#define TRUST_REASON_NONE             (0x00000000)
#define TRUST_REASON_UM_EXEMPTED      (0x00000001)
#define TRUST_REASON_FORCEFUL         (0x00010000)
#define TRUST_REASON_FALSE_POSITIVE   (TRUST_REASON_FORCEFUL | 0x00000002)

typedef struct _PM_EXCLUSION_TABLE {
    DS_SPIN_LOCK Lock;
    IDXLIST_TABLE Table;
} PM_EXCLUSION_TABLE, *PPM_EXCLUSION_TABLE;

typedef struct _PM_EXCLUSION_ENTRY {
    PETHREAD Thread;
    UINT32 UniqueId;
    UINT32 TrustReason;
} PM_EXCLUSION_ENTRY, *PPM_EXCLUSION_ENTRY;

typedef struct _DS_PROCESS_ENTRY {
    TIME_FIELDS Time;
    HANDLE ParentProcessId;
    HANDLE ProcessId;
    WCHAR Sid[MAX_SID_LENGTH];
    PM_EXCLUSION_TABLE ThreadList;
    UNICODE_STRING Path;
    UINT32 TrustReason;
    UINT32 Flags;
    UINT32 Hash;
} DS_PROCESS_ENTRY, *PDS_PROCESS_ENTRY;

typedef NTSTATUS
(*PDS_PROCESS_ENUMERATE_CALLBACK )(
    _In_ PDS_PROCESS_ENTRY ProcessEntry,
    _In_ UINT32 Flags,
    _Out_writes_bytes_to_opt_( BufferSize, *BytesReturned ) PVOID ProcessList,
    _In_ UINT32 BufferSize,
    _Out_opt_ PUINT32 BytesReturned
    );

typedef struct _SYSTEM_THREAD_INFORMATION
{
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER CreateTime;
    ULONG WaitTime;
    PVOID StartAddress;
    CLIENT_ID ClientId;
    LONG Priority;
    LONG BasePriority;
    ULONG ContextSwitches;
    ULONG ThreadState;
    ULONG WaitReason;
} SYSTEM_THREAD_INFORMATION, *PSYSTEM_THREAD_INFORMATION;
 
typedef struct _SYSTEM_EXTENDED_THREAD_INFORMATION
{
    SYSTEM_THREAD_INFORMATION ThreadInfo;
    PVOID StackBase;
    PVOID StackLimit;
    PVOID Win32StartAddress;
    PVOID TebAddress;
    ULONG Reserved1;
    ULONG Reserved2;
    ULONG Reserved3;
} SYSTEM_EXTENDED_THREAD_INFORMATION, *PSYSTEM_EXTENDED_THREAD_INFORMATION;

typedef struct _SYSTEM_PROCESS_INFORMATION
{
    ULONG NextEntryOffset;
    ULONG NumberOfThreads;
    LARGE_INTEGER SpareLi1;
    LARGE_INTEGER SpareLi2;
    LARGE_INTEGER SpareLi3;
    LARGE_INTEGER CreateTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER KernelTime;
    UNICODE_STRING ImageName;
    KPRIORITY BasePriority;
    HANDLE UniqueProcessId;
    HANDLE InheritedFromUniqueProcessId;
    ULONG HandleCount;
    ULONG SessionId;
    PVOID PageDirectoryBase;
    VM_COUNTERS VirtualMemoryCounters;
    SIZE_T PrivatePageCount;
    IO_COUNTERS IoCounters;
    SYSTEM_EXTENDED_THREAD_INFORMATION Threads[1];
} SYSTEM_PROCESS_INFORMATION, *PSYSTEM_PROCESS_INFORMATION;

typedef NTSTATUS
(*PDS_ENUM_PROCESS_CALLBACK )(
    _In_ PSYSTEM_PROCESS_INFORMATION ProcessInfo,
    _In_ PVOID Context
    );

NTSTATUS
PmInitializeProcessMonitor(
    _In_ PDEVICE_OBJECT DeviceObject
    );

VOID
PmFinalizeProcessMonitor(
    VOID
    );

PDS_PROCESS_ENTRY
PmLookupProcessEntryById(
    _In_ HANDLE ProcessId
    );

UINT32
PmGetClientTrustReason(
    VOID
    );

NTSTATUS
PmExcludeThread(
    _In_ PPM_EXCLUSION_TABLE List,
    _In_ UINT64 ThreadId,
    _In_ UINT32 TrustReason
    );

BOOLEAN
PmIsThreadExcluded(
    _In_ PPM_EXCLUSION_TABLE List,
    _In_ PETHREAD Thread,
    _Out_ PUINT32 TrustReason
    );

#endif