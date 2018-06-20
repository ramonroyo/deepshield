#include "wdk7.h"
#include "os.h"

NTSTATUS
OsVersion(
    _Out_ POS_VERSION osVersion
)
{
    NTSTATUS             status = STATUS_SUCCESS;
    RTL_OSVERSIONINFOEXW verInfo = { 0 };

    //
    // Retrieve OS version infomation
    //
    verInfo.dwOSVersionInfoSize = sizeof(verInfo);

    status = RtlGetVersion((PRTL_OSVERSIONINFOW)&verInfo);
    if (!NT_SUCCESS(status))
        return status;

    osVersion->u.f.major       = (UINT8)  verInfo.dwMajorVersion;
    osVersion->u.f.minor       = (UINT8)  verInfo.dwMinorVersion;
    osVersion->u.f.servicePack = (UINT16) verInfo.wServicePackMajor;
    osVersion->u.f.buildNumber =          verInfo.dwBuildNumber;

    return status;

}

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

#if (NTDDI_VERSION >= NTDDI_VISTA)
NTSTATUS
OsGetDebuggerDataBlock(
    _Out_ PKD_DEBUGGER_DATA_BLOCK* DebuggerData
    )
{
    NTSTATUS Status;
    ULONG BufferSize = 2 * PAGE_SIZE;
    PDUMP_HEADER DumpHeader;
    PVOID Buffer;

    *DebuggerData = NULL;
    Buffer = ExAllocatePoolWithTag( NonPagedPool, BufferSize, 'hDgM' );

    if (Buffer == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
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
#endif
