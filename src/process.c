/*++

Copyright (c) ByteHeed. All Rights Reserved.

Module Name:

    process.c

Abstract:

    This file implements the process name helper routines.

Environment:

--*/

#include "dsdef.h"
#include <wdm.h>

#if defined(WPP_EVENT_TRACING)
#include "process.tmh"
#endif

static PRTL_AVL_TABLE gProcessTree;
static KSPIN_LOCK gProcessTreeLock;
static NPAGED_LOOKASIDE_LIST gProcessTreeLookasideList;

#if (NTDDI_VERSION >= NTDDI_VISTA)
NTSTATUS
PmAllocateProcessImageName(
    _In_ HANDLE ProcessId,
    _Deref_out_ PUNICODE_STRING *ReturnedProcessName
    )
{
    NTSTATUS Status;
    PUNICODE_STRING ImageFileName = NULL;
    PUNICODE_STRING ProcessName;
    PEPROCESS Process = NULL;
    ULONG LengthNeeded = 0;

    Status = PsLookupProcessByProcessId( ProcessId, &Process );

    if (!NT_SUCCESS( Status )) {
        return Status;
    }

    Status = SeLocateProcessImageName( Process, &ImageFileName );

    if (!NT_SUCCESS( Status )) {
        goto RoutineExit;
    }
    
    LengthNeeded = sizeof( UNICODE_STRING ) + ImageFileName->MaximumLength;
    ProcessName = ExAllocatePoolWithTag( NonPagedPool,
                                         LengthNeeded,
                                         DS_TAG_PROCESS_NAME );

    if (NULL == ProcessName ) {
        Status = STATUS_NO_MEMORY;
        goto RoutineExit;
    }

    RtlInitEmptyUnicodeString( ProcessName,
                               (PWCHAR)(ProcessName + 1),
                               ImageFileName->MaximumLength );

    Status = RtlUnicodeStringCbCopyN( ProcessName,
                                      ImageFileName,
                                      ImageFileName->Length );
    NT_ASSERT( NT_SUCCESS( Status ) );

    *ReturnedProcessName = ProcessName;

RoutineExit:

    if (ImageFileName) {
        ExFreePool( ImageFileName );
    }

    if (Process) {
        ObDereferenceObject( Process );
    }

    return Status;
}
#else
NTSTATUS 
PmQueryProcessImageName(
    _In_ HANDLE ProcessHandle,
    _Inout_opt_ PUNICODE_STRING ProcessName,
    _Out_opt_ PULONG RequiredLength
    ) 
{ 
    NTSTATUS Status = STATUS_ACCESS_DENIED; 
    PUNICODE_STRING ImageName = NULL;
    ULONG ReturnedLength = 0;
    ULONG BufferLength = 0; 
    PVOID Buffer = NULL; 
    
    Status = ZwQueryInformationProcess( ProcessHandle,
                                        ProcessImageFileName,
                                        NULL,
                                        0,
                                        &ReturnedLength );

    if (STATUS_INFO_LENGTH_MISMATCH != Status) {
        return Status;
    }

    BufferLength = ReturnedLength - sizeof( UNICODE_STRING );

    if (!ARGUMENT_PRESENT( ProcessName ) ||
        ProcessName->MaximumLength < BufferLength) {

        if (ARGUMENT_PRESENT( RequiredLength )) {
            *RequiredLength = BufferLength;
        }
        return STATUS_BUFFER_OVERFLOW; 
    } 
    
    Buffer = ExAllocatePoolWithTag( NonPagedPool, 
                                    ReturnedLength,
                                    DS_TAG_PROCESS_NAME );
    if (NULL == Buffer) { 
        return STATUS_NO_MEMORY;
    } 
    
    Status = ZwQueryInformationProcess( ProcessHandle,
                                        ProcessImageFileName,
                                        Buffer,
                                        ReturnedLength,
                                        &ReturnedLength ); 
    
    if (NT_SUCCESS( Status )) {
        ImageName = (PUNICODE_STRING)Buffer;
        Status = RtlUnicodeStringCbCopyN( ProcessName,
                                          ImageName,
                                          ImageName->Length );
        NT_ASSERT( NT_SUCCESS( Status ) );
    } 

    ExFreePoolWithTag( Buffer, DS_TAG_PROCESS_NAME );
    return Status;
} 

NTSTATUS
PmAllocateProcessImageName(
    _In_ HANDLE ProcessId, 
    _Deref_out_ PUNICODE_STRING *ReturnedProcessName
    ) 
{ 
    NTSTATUS Status;
    PUNICODE_STRING ProcessName = NULL;
    HANDLE ProcessHandle = NULL;
    PEPROCESS Process = NULL;
    ULONG LengthNeeded = 0;
    
    Status = PsLookupProcessByProcessId( ProcessId, &Process ); 

    if (!NT_SUCCESS( Status )) { 
        return Status; 
    } 

    Status = ObOpenObjectByPointer( Process,
                                    OBJ_KERNEL_HANDLE,
                                    NULL,
                                    0,
                                    *PsProcessType,
                                    KernelMode,
                                    &ProcessHandle );
    if (!NT_SUCCESS( Status )) {
        goto RoutineExit;
    }

    Status = PmQueryProcessImageName( ProcessHandle,
                                      NULL,
                                      &LengthNeeded );

    if (STATUS_BUFFER_OVERFLOW != Status) {
        return Status;
    }

    ProcessName = ExAllocatePoolWithTag( NonPagedPool, 
                                         LengthNeeded + sizeof( UNICODE_STRING ),
                                         DS_TAG_PROCESS_NAME );
    if (NULL == ProcessName) {
        Status = STATUS_NO_MEMORY;
        goto RoutineExit;
    }

    RtlInitEmptyUnicodeString( ProcessName,
                               (PWCHAR)(ProcessName + 1),
                               (USHORT)LengthNeeded );

    
    RtlZeroMemory( ProcessName->Buffer, LengthNeeded );
    Status = PmQueryProcessImageName( ProcessHandle, ProcessName, NULL );

    if (NT_SUCCESS( Status )) {
        *ReturnedProcessName = ProcessName;
    }
    
RoutineExit:

    if (!NT_SUCCESS( Status )) {
        if (ProcessName) {
            ExFreePoolWithTag( ProcessName, DS_TAG_PROCESS_NAME );
        }
    }

    if (ProcessHandle) {
        ZwClose( ProcessHandle );
    }

    if (Process) {
        ObDereferenceObject( Process );
    }

    return Status; 
}

#endif

VOID
PmFreeProcessImageName(
    _Inout_ PUNICODE_STRING ProcessName
    )
{
    ExFreePoolWithTag( ProcessName, DS_TAG_PROCESS_NAME );
}

NTSTATUS
PmParseProcessImageName(
    _In_ PUNICODE_STRING ProcessName,
    _Inout_ PUNICODE_STRING ReturnedFileName
    )
{
    PWCHAR FileName = NULL;
    NTSTATUS Status = STATUS_NOT_FOUND;
    USHORT ProcessNameLength = 0;

    NT_ASSERT( ProcessName->Length > 0 );
    FileName = (PWCHAR)
        ((PCHAR) ProcessName->Buffer + ProcessName->Length);

    while (FileName > ProcessName->Buffer) {
        if (OBJ_NAME_PATH_SEPARATOR == *(--FileName)) {

            ReturnedFileName->Buffer = FileName++;
            ReturnedFileName->MaximumLength = ProcessNameLength;
            ReturnedFileName->Length = ProcessNameLength;

            Status = STATUS_SUCCESS;
            break;
        }
        else {
            ProcessNameLength += sizeof( WCHAR );
        }
    }

    return Status;
}

UINT32
PmGetImageNameHash(
    _In_ PUNICODE_STRING ImageName
    )
{
    PWCHAR ImageBuffer = ImageName->Buffer;
    UINT32 Hash = 0;
    UINT32 Idx = 0;

    for(; Idx < (ImageName->Length / sizeof( WCHAR )); ImageBuffer++, Idx++) {
        Hash = tolower( *ImageBuffer ) + (Hash << 6) + (Hash << 16) - Hash;
    }

    return Hash;
}

NTSTATUS
PmGetProcessSid(
    _In_ HANDLE ProcessId,
    _Inout_ PUNICODE_STRING Sid
    )
{
    NTSTATUS Status;
    //HANDLE ProcessHandle = NULL;
    HANDLE ProcessToken = NULL;
    PACCESS_TOKEN Token = NULL;
    PTOKEN_USER ProcessUser = NULL;
    PEPROCESS Process = NULL;
    ULONG ProcessUserBytes = 0;

    UNREFERENCED_PARAMETER( ProcessId );

    if (!Sid) {
        return STATUS_INVALID_PARAMETER;
    }

    Status = PsLookupProcessByProcessId( ProcessId, &Process );

    if (!NT_SUCCESS( Status )) {
        return Status;
    }
   
    Token = PsReferencePrimaryToken( Process );
    Status = ObOpenObjectByPointer( Token, 
                                    OBJ_KERNEL_HANDLE,
                                    NULL,
                                    TOKEN_QUERY,
                                    *SeTokenObjectType,
                                    KernelMode,
                                    &ProcessToken );

    if (!NT_SUCCESS( Status )) {
        goto RoutineExit;
    }

    PsDereferencePrimaryToken( Token );

    /*
    Status = ObOpenObjectByPointer( Process,
                                    OBJ_KERNEL_HANDLE,
                                    NULL,
                                    0,
                                    *PsProcessType,
                                    KernelMode,
                                    &ProcessHandle );
    if (!NT_SUCCESS( Status )) {
        goto RoutineExit;
    }

    Status = ZwOpenProcessTokenEx( ZwCurrentProcess(),
                                   GENERIC_READ,
                                   OBJ_KERNEL_HANDLE,
                                   &ProcessToken );

    if (!NT_SUCCESS( Status )) {
        TraceEvents( TRACE_LEVEL_WARNING, TRACE_PROCESS,
                     "Cannot open token for process %d (Status: %!STATUS!)\n",
                     ProcessId,
                     Status );

        goto RoutineExit;
    }
    */

    Status = ZwQueryInformationToken( ProcessToken,
                                      TokenUser,
                                      NULL,
                                      0, 
                                      &ProcessUserBytes );

    if (Status != STATUS_BUFFER_TOO_SMALL) {
        TraceEvents( TRACE_LEVEL_WARNING, TRACE_PROCESS,
                     "Cannot get token information size for process %p (Status: %!STATUS!)\n",
                     ProcessId,
                     Status );

        goto RoutineExit;
    }

    ProcessUser = (PTOKEN_USER)
        ExAllocatePoolWithTag( NonPagedPool, ProcessUserBytes, DS_TAG_TOKEN_USER );

    if (ProcessUser == NULL) {
        TraceEvents( TRACE_LEVEL_WARNING, TRACE_PROCESS,
                     "Cannot allocate %d token information bytes for process %p\n",
                     ProcessUserBytes,
                     ProcessId );

        Status = STATUS_NO_MEMORY;
        goto RoutineExit;
    }

    Status = ZwQueryInformationToken( ProcessToken,
                                      TokenUser,
                                      ProcessUser,
                                      ProcessUserBytes,
                                      &ProcessUserBytes );

    if (!NT_SUCCESS( Status )) {
        TraceEvents( TRACE_LEVEL_WARNING, TRACE_PROCESS,
                     "Cannot get token information for process %p (Status: %!STATUS!)\n",
                     ProcessId,
                     Status );

        goto RoutineExit;
    }

    Status = RtlConvertSidToUnicodeString( Sid, ProcessUser->User.Sid, TRUE );

    if (!NT_SUCCESS( Status )) {
        TraceEvents( TRACE_LEVEL_WARNING, TRACE_PROCESS,
                     "Cannot convert SID to string for process %p (Status: %!STATUS!)\n",
                     ProcessId,
                     Status );

        goto RoutineExit;
    }

RoutineExit:

    /*
    if (ProcessHandle) {
        ZwClose( ProcessHandle );
    }*/

    if (ProcessToken) {
        ZwClose( ProcessToken );
    }
    if (ProcessUser) {
        ExFreePoolWithTag( ProcessUser, DS_TAG_TOKEN_USER );
    }

    return Status;
}

NTSTATUS
PmQueryTokenInformation(
    _In_ HANDLE TokenHandle,
    _In_ TOKEN_INFORMATION_CLASS TokenInformationClass,
    _Out_ PVOID *TokenInformation
    )
{
    NTSTATUS Status = STATUS_NO_MEMORY;
    PVOID Buffer = NULL;
    ULONG ReturnLength = 0;


    Status = ZwQueryInformationToken( TokenHandle,
                                      TokenInformationClass,
                                      NULL,
                                      0, 
                                      &ReturnLength );

    if (STATUS_BUFFER_TOO_SMALL != Status) {
        return Status;
    }

    Buffer = ExAllocatePoolWithTag( PagedPool, ReturnLength, 'iTsD' );

    if (Buffer) {
        Status = ZwQueryInformationToken( TokenHandle,
                                          TokenInformationClass,
                                          Buffer,
                                          ReturnLength,
                                          &ReturnLength );

        if (NT_SUCCESS( Status ) ) {
            *TokenInformation = Buffer;
        }
        else {
            ExFreePoolWithTag( Buffer, 'iTsD' );
        }
    }

    return Status;
}

VOID
PmFreeTokenInformation(
    _In_ PVOID TokenInformation
    )
{
    ExFreePoolWithTag( TokenInformation, 'iTsD' );
}

NTSTATUS 
PmGetTokenMandarotyLevel(
    _In_ HANDLE TokenHandle,
    _Out_opt_ PMANDATORY_LEVEL MandatoryLevel,
    _Out_opt_ PWSTR *MandatoryString
    )
{
    NTSTATUS Status;
    PTOKEN_MANDATORY_LABEL MandatoryLabel = NULL;
    UINT32 IntegrityLevel;

    Status = PmQueryTokenInformation( TokenHandle,
                                      TokenIntegrityLevel,
                                      &MandatoryLabel );

    if (!NT_SUCCESS( Status )) {
       return Status;
    }

    IntegrityLevel = *RtlSubAuthoritySid( MandatoryLabel->Label.Sid,
        (ULONG)(*RtlSubAuthorityCountSid( MandatoryLabel->Label.Sid ) - 1) );

    PmFreeTokenInformation( MandatoryLabel );

    switch ( IntegrityLevel ) {

        case SECURITY_MANDATORY_UNTRUSTED_RID:
            *MandatoryLevel = MandatoryLevelUntrusted;
            *MandatoryString = L"UntrustedLevel";
            break;

        case SECURITY_MANDATORY_LOW_RID:
            *MandatoryLevel = MandatoryLevelLow;
            *MandatoryString = L"LowLevel";
            break;

        case SECURITY_MANDATORY_MEDIUM_RID:
            *MandatoryLevel = MandatoryLevelMedium;
            *MandatoryString = L"MediumLevel";
            break;

        case SECURITY_MANDATORY_HIGH_RID:
            *MandatoryLevel = MandatoryLevelHigh;
            *MandatoryString = L"HighLevel";
            break;

        case SECURITY_MANDATORY_SYSTEM_RID:
            *MandatoryLevel = MandatoryLevelSystem;
            *MandatoryString = L"SystemLevel";
            break;

        case SECURITY_MANDATORY_PROTECTED_PROCESS_RID:
            *MandatoryLevel = MandatoryLevelSecureProcess;
            *MandatoryString = L"ProtectedLevel";
            break;

        default:
            return STATUS_UNSUCCESSFUL;
    }

    return Status;
}

RTL_GENERIC_COMPARE_RESULTS
PmCompareNode(
    _In_ PRTL_AVL_TABLE Table,
    _In_ PVOID Lhs,
    _In_ PVOID Rhs
    )
{
    PDS_PROCESS_ENTRY FirstEntry = (PDS_PROCESS_ENTRY) Lhs;
    PDS_PROCESS_ENTRY SecondEntry = (PDS_PROCESS_ENTRY) Rhs;

    UNREFERENCED_PARAMETER( Table );

    if (FirstEntry->ProcessId > SecondEntry->ProcessId) {
        return GenericGreaterThan;
    }
    else if (FirstEntry->ProcessId < SecondEntry->ProcessId) {
        return GenericLessThan;
    }
    else {
        return GenericEqual;
    }
}

PVOID
PmAllocateNode(
    _In_ PRTL_AVL_TABLE Table,
    _In_ CLONG ByteSize
    )
{
    UNREFERENCED_PARAMETER( Table );

    return ExAllocatePoolWithTag( NonPagedPool, 
                                  ByteSize,
                                  DS_TAG_PROCESS_ENTRY );
}

VOID
PmFreeNode(
    _In_ PRTL_AVL_TABLE Table,
    _In_ __drv_freesMem( Mem ) _Post_invalid_ PVOID Buffer
    )
{
    UNREFERENCED_PARAMETER( Table );
    ExFreePoolWithTag( Buffer, DS_TAG_PROCESS_ENTRY );
}

PRTL_AVL_TABLE
PmInitializeProcessTable(
    VOID
    )
{
    PRTL_AVL_TABLE Table = (PRTL_AVL_TABLE)
        ExAllocatePoolWithTag( NonPagedPool, 
                               sizeof( RTL_AVL_TABLE ),
                               DS_TAG_PROCESS_ENTRY );

    if (NULL == Table) {
        return NULL;
    }

    RtlInitializeGenericTableAvl( Table,
                                  PmCompareNode,
                                  PmAllocateNode,
                                  PmFreeNode,
                                  NULL );
    return Table;
}

VOID
PmClearProcessTable(
    _In_ PRTL_AVL_TABLE Table
    )
{
    PVOID Entry;

    while (!RtlIsGenericTableEmptyAvl( Table ))
    {
        Entry = RtlGetElementGenericTableAvl( Table, 0 );
        RtlDeleteElementGenericTableAvl( Table, Entry );
    }
}

NTSTATUS
PmEnumerateEntryProcessTable(
    _In_ PRTL_AVL_TABLE Table,
    _In_ PDS_PROCESS_ENUMERATE_CALLBACK EnumerationRoutine,
    _In_ UINT32 Flags,
    _Out_writes_bytes_to_opt_( BufferSize, *BytesReturned ) PVOID ProcessList,
    _In_ UINT32 BufferSize,
    _Out_opt_ PUINT32 BytesReturned
    )
{
    PVOID Entry;
    NTSTATUS Status = STATUS_SUCCESS;

    for (Entry = RtlEnumerateGenericTableAvl( Table, TRUE );
         Entry != NULL;
         Entry = RtlEnumerateGenericTableAvl( Table, FALSE ) ) {

        Status = EnumerationRoutine( Entry,
                                     Flags,
                                     ProcessList,
                                     BufferSize,
                                     BytesReturned );
    }

    return Status;
}

VOID
PmDeleteProcessTable(
    _In_ PRTL_AVL_TABLE Table
    )
{
    PmClearProcessTable( Table );
    ExFreePoolWithTag( Table, DS_TAG_PROCESS_ENTRY );
}

NTSTATUS
PmAddProcessEntry(
    _In_ PRTL_AVL_TABLE Table,
    _In_ PDS_PROCESS_ENTRY ProcessEntry
    )
{
    NTSTATUS Status;
    PDS_PROCESS_ENTRY InsertedEntry = NULL;
    BOOLEAN NewElement = FALSE;

    NT_ASSERT( Table );
    NT_ASSERT( ProcessEntry );

    //
    //  FIX: path is lost since the copied string pointer is stale.
    //

    InsertedEntry = RtlInsertElementGenericTableAvl( 
                        Table,
                        ProcessEntry,
                        sizeof( DS_PROCESS_ENTRY ),
                        &NewElement );

    if (NULL != InsertedEntry) {
        if (NewElement == TRUE ) {
            TraceEvents( TRACE_LEVEL_INFORMATION, TRACE_PROCESS,
                         "Process %p added to cache (Path: %wZ, Hash: %x)\n",
                         ProcessEntry->ProcessId,
                         &ProcessEntry->Path,
                         ProcessEntry->Hash );

            Status = STATUS_SUCCESS;
        }
        else {
            TraceEvents( TRACE_LEVEL_WARNING, TRACE_PROCESS,
                         "Process %p already cached (Path: %wZ, Hash: %x)\n",
                         ProcessEntry->ProcessId,
                         &ProcessEntry->Path,
                         ProcessEntry->Hash );

            //
            //  The entry already exists.
            //
            Status = STATUS_OBJECTID_EXISTS;
        }
    } else {
        Status = STATUS_INTERNAL_ERROR;
    }

    return Status;
}

PDS_PROCESS_ENTRY
PmLookupProcessEntryById(
    _In_ HANDLE ProcessId
    )
{
    DS_PROCESS_ENTRY ProcessEntry = { 0 };
    ProcessEntry.ProcessId = ProcessId;

    return RtlLookupElementGenericTableAvl( gProcessTree, &ProcessEntry );
}

NTSTATUS
PmRemoveProcessEntry(
    _In_ PDS_PROCESS_ENTRY ProcessEntry
    )
{ 
    NTSTATUS Status = STATUS_NOT_FOUND;
    BOOLEAN Deleted;
    
    Deleted = RtlDeleteElementGenericTableAvl( gProcessTree, ProcessEntry );

    if (Deleted) {
        Status = STATUS_SUCCESS;
    }

    return Status;
}

NTSTATUS
PmGetExemptedProcessListCallback(
    _In_ PDS_PROCESS_ENTRY ProcessEntry,
    _In_ UINT32 Flags,
    _Out_writes_bytes_to_opt_( BufferSize, *BytesReturned ) PVOID ProcessList,
    _In_ UINT32 BufferSize,
    _Out_opt_ PUINT32 BytesReturned
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PUINT32 ProcessIdList = ProcessList;
    UINT32 ProcessCount = 0;

    UNREFERENCED_PARAMETER( Flags );

    if (FlagOn( ProcessEntry->TrustLevel, TRUST_LEVEL_EXEMPTED )) {

        if ((ProcessCount + 1) * sizeof( UINT32 ) <= BufferSize) {
            ProcessIdList[ProcessCount ] = PtrToUlong( ProcessEntry->ProcessId );

        } else {
            Status = STATUS_BUFFER_TOO_SMALL;
        }

        ProcessCount++;
    }

    if (ARGUMENT_PRESENT( BytesReturned )) {
        *BytesReturned = ProcessCount * sizeof( UINT32 );
    }

    return Status;
}

NTSTATUS
PmGetExemptedProcessList(
    _In_ PRTL_AVL_TABLE Table,
    _Out_writes_bytes_to_opt_( BufferSize, *BytesReturned ) PVOID ProcessList,
    _In_ UINT32 BufferSize,
    _Out_opt_ PUINT32 BytesReturned
    )
{
    NTSTATUS Status;
    Status = PmEnumerateEntryProcessTable( Table,
                                           PmGetExemptedProcessListCallback,
                                           0,
                                           ProcessList,
                                           BufferSize,
                                           BytesReturned );
    return Status;
}

VOID
DsProcessNotifyCallback(
    _In_ HANDLE ParentId, 
    _In_ HANDLE ProcessId,
    _In_ BOOLEAN Create
    )
{
    NTSTATUS Status;
    UNICODE_STRING Sid;
    PUNICODE_STRING ProcessPath = NULL;
    PDS_PROCESS_ENTRY ProcessEntry = NULL;
    LARGE_INTEGER SystemTime;
    LARGE_INTEGER LocalTime;
    TIME_FIELDS TimeFields;
    KLOCK_QUEUE_HANDLE LockHandle;
    ULONG EntryLength;

    if (Create) {

        Status = PmAllocateProcessImageName( ProcessId, &ProcessPath );
        if (!NT_SUCCESS( Status )) {
            goto RoutineExit;
        }

        EntryLength = sizeof( DS_PROCESS_ENTRY ) + ProcessPath->Length;

        ProcessEntry = ExAllocatePoolWithTag( NonPagedPool, 
                                              EntryLength,
                                              DS_TAG_PROCESS_ENTRY );
        if (NULL == ProcessEntry) {
            return;
        }

        RtlZeroMemory( ProcessEntry, sizeof( DS_PROCESS_ENTRY ) );

        RtlInitEmptyUnicodeString( &ProcessEntry->Path,
                                   (PWCHAR)(ProcessEntry + 1),
                                   ProcessPath->Length );

        RtlUnicodeStringCbCopyN( &ProcessEntry->Path,
                                 ProcessPath,
                                 ProcessPath->Length );

        KeQuerySystemTime( &SystemTime );
        ExSystemTimeToLocalTime( &SystemTime, &LocalTime );
        RtlTimeToTimeFields( &LocalTime, &TimeFields );

        RtlCopyMemory( &ProcessEntry->Time,
                       &TimeFields, 
                       sizeof( TIME_FIELDS ) );
        
        ProcessEntry->ParentProcessId = ParentId;
        ProcessEntry->ProcessId = ProcessId;

        Status = PmGetProcessSid( ProcessId , &Sid );
        if (!NT_SUCCESS( Status )) {
            goto RoutineExit;
        }

        Status = RtlStringCchCopyUnicodeString( &ProcessEntry->Sid[0],
                                                MAX_SID_LENGTH,
                                                &Sid );
        NT_ASSERT( NT_SUCCESS( Status ) );
        RtlFreeUnicodeString( &Sid );

        //
        //  Query the rules that were provided by the helper service.
        //
        //  TODO: PmEvaluateRules();
        //

        KeAcquireInStackQueuedSpinLock( &gProcessTreeLock, &LockHandle );
        Status = PmAddProcessEntry( gProcessTree, ProcessEntry );
        KeReleaseInStackQueuedSpinLock( &LockHandle );
    }
    else {

        KeAcquireInStackQueuedSpinLock( &gProcessTreeLock, &LockHandle );
        ProcessEntry = PmLookupProcessEntryById( ProcessId );

        if (ProcessEntry) {
            Status = PmRemoveProcessEntry( ProcessEntry );

            NT_ASSERT( STATUS_SUCCESS == Status );
            ProcessEntry = NULL;
        }

        KeReleaseInStackQueuedSpinLock( &LockHandle );
    }

RoutineExit:

    if (ProcessPath) {
        PmFreeProcessImageName( ProcessPath );
    }

    if (ProcessEntry) {
        ExFreePoolWithTag( ProcessEntry, DS_TAG_PROCESS_ENTRY );
    }
}

NTSTATUS
PmInitializeProcessMonitor(
    _In_ PDEVICE_OBJECT DeviceObject
    )
{
    NTSTATUS Status;
    BOOLEAN CreateNotifyStarted = FALSE;

    UNREFERENCED_PARAMETER( DeviceObject );

    KeInitializeSpinLock( &gProcessTreeLock );
    ExInitializeNPagedLookasideList( 
                              &gProcessTreeLookasideList,
                              NULL,
                              NULL,
                              0,
                              sizeof( DS_PROCESS_ENTRY ) + MAX_PROCESS_NAME,
                              DS_TAG_PROCESS_ENTRY,
                              0 );

    gProcessTree = PmInitializeProcessTable();

    if (NULL == gProcessTree) {
        Status = STATUS_NO_MEMORY;
        goto RoutineExit;
    }

    Status = PsSetCreateProcessNotifyRoutine( DsProcessNotifyCallback, FALSE );

    if (!NT_SUCCESS( Status )) {
        goto RoutineExit;
    }

    CreateNotifyStarted = TRUE;

    //
    //  TODO: Capture the info for already existing processes.
    //

RoutineExit:

    if (!NT_SUCCESS( Status ) ) {

        if (gProcessTree) {
            PmDeleteProcessTable( gProcessTree );
            gProcessTree = NULL;
        }

        if (CreateNotifyStarted) {
            PsSetCreateProcessNotifyRoutine( DsProcessNotifyCallback, TRUE );
        }

        ExDeleteNPagedLookasideList( &gProcessTreeLookasideList );
    }

    return Status;
}

VOID
PmFinalizeProcessMonitor(
    VOID
    )
{
    NTSTATUS Status;
    KLOCK_QUEUE_HANDLE LockHandle;

    Status = PsSetCreateProcessNotifyRoutine( DsProcessNotifyCallback, TRUE );
    NT_ASSERT( NT_SUCCESS( Status ) );

    KeAcquireInStackQueuedSpinLock( &gProcessTreeLock, &LockHandle );
    PmDeleteProcessTable( gProcessTree );
    gProcessTree = NULL;

    KeReleaseInStackQueuedSpinLock( &LockHandle );
    ExDeleteNPagedLookasideList( &gProcessTreeLookasideList );
}
