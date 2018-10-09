#include "wdk7.h"
#include "os.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, OsVersion)
#pragma alloc_text(PAGE, OsVerifyBuildNumber)
#endif

_IRQL_requires_max_( PASSIVE_LEVEL )
NTSTATUS
OsVersion(
    _Out_ POS_VERSION osVersion
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    RTL_OSVERSIONINFOEXW verInfo = { 0 };

    PAGED_CODE();
    verInfo.dwOSVersionInfoSize = sizeof(verInfo);

    Status = RtlGetVersion((PRTL_OSVERSIONINFOW)&verInfo);
    
    if (!NT_SUCCESS( Status )) {
        return Status;
    }

    osVersion->u.f.major       = (UINT8)  verInfo.dwMajorVersion;
    osVersion->u.f.minor       = (UINT8)  verInfo.dwMinorVersion;
    osVersion->u.f.servicePack = (UINT16) verInfo.wServicePackMajor;
    osVersion->u.f.buildNumber =          verInfo.dwBuildNumber;

    return Status;
}

_IRQL_requires_max_( PASSIVE_LEVEL )
BOOLEAN
OsVerifyBuildNumber(
    _In_ ULONG BuildNumber
    )
/*++
Routine Description:

    This routine verify the presence of an OS version that matches or it is
    higher than a given build number.

Arguments:
    
    BuildNumber - input build number to verify.
    
Return value:

    TRUE if result is positive, FALSE otherwise.

--*/
{

    NTSTATUS Status;
    RTL_OSVERSIONINFOEXW VersionInfo = {0};
    ULONGLONG ConditionMask = 0;

    PAGED_CODE();
   
    VersionInfo.dwOSVersionInfoSize = sizeof(VersionInfo);
    VersionInfo.dwBuildNumber = BuildNumber;

    VER_SET_CONDITION( ConditionMask, VER_BUILDNUMBER, VER_GREATER_EQUAL );

    Status = RtlVerifyVersionInfo( &VersionInfo, 
                                   VER_BUILDNUMBER,
                                   ConditionMask );

    if (NT_SUCCESS ( Status )) {
        return TRUE;
    }

    return FALSE;
}

#if (NTDDI_VERSION >= NTDDI_VISTA) && defined(_WIN64)
NTSTATUS
OsGetDebuggerDataBlock(
    _Deref_out_ PKD_DEBUGGER_DATA_BLOCK* DebuggerData
    )
{
    NTSTATUS Status;
    ULONG BufferSize = 2 * PAGE_SIZE;
    PDUMP_HEADER DumpHeader;
    PVOID Buffer;

    *DebuggerData = NULL;
    Buffer = ExAllocatePoolWithTag( NonPagedPool, BufferSize, 'hDgM' );

    if (Buffer == NULL) {
        return STATUS_NO_MEMORY;
    }

    Status = KeInitializeCrashDumpHeader( DUMP_TYPE_FULL,
                                          0, 
                                          Buffer,
                                          BufferSize,
                                          NULL );
    if (NT_SUCCESS( Status )) {

        DumpHeader = (PDUMP_HEADER)Buffer;
        *DebuggerData = 
            (PKD_DEBUGGER_DATA_BLOCK)DumpHeader->KdDebuggerDataBlock;
        
        NT_ASSERT( *(PULONG_PTR)((*DebuggerData)->MmPfnDatabase) 
                   == DumpHeader->PfnDataBase );
    }

    ExFreePoolWithTag( Buffer, 'hDgM' );
    return Status;
}

NTSTATUS
OsGetPfnDatabase(
    _Out_ PUINT64 PfnDataBase
    )
{
    NTSTATUS Status;
    ULONG BufferSize = 2 * PAGE_SIZE;
    PVOID Buffer;

    *PfnDataBase = 0;
    Buffer = ExAllocatePoolWithTag( NonPagedPool, BufferSize, 'hDgM' );

    if (Buffer == NULL) {
        return STATUS_NO_MEMORY;
    }

    Status = KeInitializeCrashDumpHeader( DUMP_TYPE_FULL,
                                          0, 
                                          Buffer,
                                          BufferSize,
                                          NULL );
    if (NT_SUCCESS( Status )) {
        *PfnDataBase = ((PDUMP_HEADER)Buffer)->PfnDataBase;
    }

    ExFreePoolWithTag( Buffer, 'hDgM' );
    return Status;
}
#endif
