/*++

Copyright (C) ByteHeed. All Rights Reserved

Module Name:

    process.h

--*/

#ifndef _PROCESS_
#define _PROCESS_

#include <ntifs.h>
#include "wdk7.h"

#define DS_TAG_PROCESS_ENTRY     'ePsD'
#define DS_TAG_PROCESS_NAME      'nPsD'
#define DS_TAG_TOKEN_USER        'uTsD'

#define MAX_PROCESS_NAME        512L
#define MAX_SID_LENGTH          192L


#define TRUST_LEVEL_NONE           (0x00000001)
#define TRUST_LEVEL_EXEMPTED       (0x00000002)
#define TRUST_LEVEL_FORCEFUL       (0x00010000)
#define TRUST_LEVEL_MASSIVE        (TRUST_LEVEL_FORCEFUL | TRUST_LEVEL_EXEMPTED)

typedef struct _DS_PROCESS_ENTRY {
    TIME_FIELDS Time;
    HANDLE ParentProcessId;
    HANDLE ProcessId;
    WCHAR Sid[MAX_SID_LENGTH];
    UNICODE_STRING Path;
    UINT32 TrustLevel;
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

extern
NTSTATUS
ZwQueryInformationProcess(
    _In_ HANDLE ProcessHandle,
    _In_ PROCESSINFOCLASS ProcessInformationClass,
    _Out_ PVOID ProcessInformation,
    _In_ ULONG ProcessInformationLength,
    _Out_opt_ PULONG ReturnLength
    );

NTSTATUS
PmInitializeProcessMonitor(
    _In_ PDEVICE_OBJECT DeviceObject
    );

VOID
PmFinalizeProcessMonitor(
    VOID
    );

#endif