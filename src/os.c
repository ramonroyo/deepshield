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
