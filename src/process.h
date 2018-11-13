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

typedef struct _DS_PROCESS_ENTRY {
    TIME_FIELDS Time;
    HANDLE ParentProcessId;
    HANDLE ProcessId;
    WCHAR Sid[MAX_SID_LENGTH];
    UNICODE_STRING Path;
    UINT32 Flags;
    UINT32 Hash;
} DS_PROCESS_ENTRY, *PDS_PROCESS_ENTRY;

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