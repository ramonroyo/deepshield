#ifndef __OS_H__
#define __OS_H__

#include <ntifs.h>

#pragma warning(disable:4201 4214)

typedef struct _DBGKD_DEBUG_DATA_HEADER32 {

    LIST_ENTRY32 List;
    ULONG           OwnerTag;
    ULONG           Size;

} DBGKD_DEBUG_DATA_HEADER32, *PDBGKD_DEBUG_DATA_HEADER32;

typedef struct _KDDEBUGGER_DATA32 {

    DBGKD_DEBUG_DATA_HEADER32 Header;
    ULONG   KernBase;
    ULONG   BreakpointWithStatus;       // address of breakpoint
    ULONG   SavedContext;
    USHORT  ThCallbackStack;            // offset in thread data
    USHORT  NextCallback;               // saved pointer to next callback frame
    USHORT  FramePointer;               // saved frame pointer
    USHORT  PaeEnabled:1;
    ULONG   KiCallUserMode;             // kernel routine
    ULONG   KeUserCallbackDispatcher;   // address in ntdll

    ULONG   PsLoadedModuleList;
    ULONG   PsActiveProcessHead;
    ULONG   PspCidTable;

    ULONG   ExpSystemResourcesList;
    ULONG   ExpPagedPoolDescriptor;
    ULONG   ExpNumberOfPagedPools;

    ULONG   KeTimeIncrement;
    ULONG   KeBugCheckCallbackListHead;
    ULONG   KiBugcheckData;

    ULONG   IopErrorLogListHead;

    ULONG   ObpRootDirectoryObject;
    ULONG   ObpTypeObjectType;

    ULONG   MmSystemCacheStart;
    ULONG   MmSystemCacheEnd;
    ULONG   MmSystemCacheWs;

    ULONG   MmPfnDatabase;
    ULONG   MmSystemPtesStart;
    ULONG   MmSystemPtesEnd;
    ULONG   MmSubsectionBase;
    ULONG   MmNumberOfPagingFiles;

    ULONG   MmLowestPhysicalPage;
    ULONG   MmHighestPhysicalPage;
    ULONG   MmNumberOfPhysicalPages;

    ULONG   MmMaximumNonPagedPoolInBytes;
    ULONG   MmNonPagedSystemStart;
    ULONG   MmNonPagedPoolStart;
    ULONG   MmNonPagedPoolEnd;

    ULONG   MmPagedPoolStart;
    ULONG   MmPagedPoolEnd;
    ULONG   MmPagedPoolInformation;
    ULONG   MmPageSize;

    ULONG   MmSizeOfPagedPoolInBytes;

    ULONG   MmTotalCommitLimit;
    ULONG   MmTotalCommittedPages;
    ULONG   MmSharedCommit;
    ULONG   MmDriverCommit;
    ULONG   MmProcessCommit;
    ULONG   MmPagedPoolCommit;
    ULONG   MmExtendedCommit;

    ULONG   MmZeroedPageListHead;
    ULONG   MmFreePageListHead;
    ULONG   MmStandbyPageListHead;
    ULONG   MmModifiedPageListHead;
    ULONG   MmModifiedNoWritePageListHead;
    ULONG   MmAvailablePages;
    ULONG   MmResidentAvailablePages;

    ULONG   PoolTrackTable;
    ULONG   NonPagedPoolDescriptor;

    ULONG   MmHighestUserAddress;
    ULONG   MmSystemRangeStart;
    ULONG   MmUserProbeAddress;

    ULONG   KdPrintCircularBuffer;
    ULONG   KdPrintCircularBufferEnd;
    ULONG   KdPrintWritePointer;
    ULONG   KdPrintRolloverCount;

    ULONG   MmLoadedUserImageList;

} KDDEBUGGER_DATA32, *PKDDEBUGGER_DATA32;

typedef struct _DBGKD_DEBUG_DATA_HEADER64 {

    //
    // Link to other blocks
    //

    LIST_ENTRY64 List;

    //
    // This is a unique tag to identify the owner of the block.
    // If your component only uses one pool tag, use it for this, too.
    //

    ULONG           OwnerTag;

    //
    // This must be initialized to the size of the data block,
    // including this structure.
    //

    ULONG           Size;

} DBGKD_DEBUG_DATA_HEADER64, *PDBGKD_DEBUG_DATA_HEADER64;

typedef struct _KDDEBUGGER_DATA64 {

    DBGKD_DEBUG_DATA_HEADER64 Header;

    //
    // Base address of kernel image
    //

    ULONG64   KernBase;

    //
    // DbgBreakPointWithStatus is a function which takes an argument
    // and hits a breakpoint.  This field contains the address of the
    // breakpoint instruction.  When the debugger sees a breakpoint
    // at this address, it may retrieve the argument from the first
    // argument register, or on x86 the eax register.
    //

    ULONG64   BreakpointWithStatus;       // address of breakpoint

    //
    // Address of the saved context record during a bugcheck
    //
    // N.B. This is an automatic in KeBugcheckEx's frame, and
    // is only valid after a bugcheck.
    //

    ULONG64   SavedContext;

    //
    // help for walking stacks with user callbacks:
    //

    //
    // The address of the thread structure is provided in the
    // WAIT_STATE_CHANGE packet.  This is the offset from the base of
    // the thread structure to the pointer to the kernel stack frame
    // for the currently active usermode callback.
    //

    USHORT  ThCallbackStack;            // offset in thread data

    //
    // these values are offsets into that frame:
    //

    USHORT  NextCallback;               // saved pointer to next callback frame
    USHORT  FramePointer;               // saved frame pointer

    //
    // pad to a quad boundary
    //
    USHORT  PaeEnabled:1;

    //
    // Address of the kernel callout routine.
    //

    ULONG64   KiCallUserMode;             // kernel routine

    //
    // Address of the usermode entry point for callbacks.
    //

    ULONG64   KeUserCallbackDispatcher;   // address in ntdll


    //
    // Addresses of various kernel data structures and lists
    // that are of interest to the kernel debugger.
    //

    ULONG64   PsLoadedModuleList;
    ULONG64   PsActiveProcessHead;
    ULONG64   PspCidTable;

    ULONG64   ExpSystemResourcesList;
    ULONG64   ExpPagedPoolDescriptor;
    ULONG64   ExpNumberOfPagedPools;

    ULONG64   KeTimeIncrement;
    ULONG64   KeBugCheckCallbackListHead;
    ULONG64   KiBugcheckData;

    ULONG64   IopErrorLogListHead;

    ULONG64   ObpRootDirectoryObject;
    ULONG64   ObpTypeObjectType;

    ULONG64   MmSystemCacheStart;
    ULONG64   MmSystemCacheEnd;
    ULONG64   MmSystemCacheWs;

    ULONG64   MmPfnDatabase;
    ULONG64   MmSystemPtesStart;
    ULONG64   MmSystemPtesEnd;
    ULONG64   MmSubsectionBase;
    ULONG64   MmNumberOfPagingFiles;

    ULONG64   MmLowestPhysicalPage;
    ULONG64   MmHighestPhysicalPage;
    ULONG64   MmNumberOfPhysicalPages;

    ULONG64   MmMaximumNonPagedPoolInBytes;
    ULONG64   MmNonPagedSystemStart;
    ULONG64   MmNonPagedPoolStart;
    ULONG64   MmNonPagedPoolEnd;

    ULONG64   MmPagedPoolStart;
    ULONG64   MmPagedPoolEnd;
    ULONG64   MmPagedPoolInformation;
    ULONG64   MmPageSize;

    ULONG64   MmSizeOfPagedPoolInBytes;

    ULONG64   MmTotalCommitLimit;
    ULONG64   MmTotalCommittedPages;
    ULONG64   MmSharedCommit;
    ULONG64   MmDriverCommit;
    ULONG64   MmProcessCommit;
    ULONG64   MmPagedPoolCommit;
    ULONG64   MmExtendedCommit;

    ULONG64   MmZeroedPageListHead;
    ULONG64   MmFreePageListHead;
    ULONG64   MmStandbyPageListHead;
    ULONG64   MmModifiedPageListHead;
    ULONG64   MmModifiedNoWritePageListHead;
    ULONG64   MmAvailablePages;
    ULONG64   MmResidentAvailablePages;

    ULONG64   PoolTrackTable;
    ULONG64   NonPagedPoolDescriptor;

    ULONG64   MmHighestUserAddress;
    ULONG64   MmSystemRangeStart;
    ULONG64   MmUserProbeAddress;

    ULONG64   KdPrintCircularBuffer;
    ULONG64   KdPrintCircularBufferEnd;
    ULONG64   KdPrintWritePointer;
    ULONG64   KdPrintRolloverCount;

    ULONG64   MmLoadedUserImageList;

    // NT 5.1 Addition

    ULONG64   NtBuildLab;
    ULONG64   KiNormalSystemCall;

    // NT 5.0 hotfix addition

    ULONG64   KiProcessorBlock;
    ULONG64   MmUnloadedDrivers;
    ULONG64   MmLastUnloadedDriver;
    ULONG64   MmTriageActionTaken;
    ULONG64   MmSpecialPoolTag;
    ULONG64   KernelVerifier;
    ULONG64   MmVerifierData;
    ULONG64   MmAllocatedNonPagedPool;
    ULONG64   MmPeakCommitment;
    ULONG64   MmTotalCommitLimitMaximum;
    ULONG64   CmNtCSDVersion;

    // NT 5.1 Addition

    ULONG64   MmPhysicalMemoryBlock;
    ULONG64   MmSessionBase;
    ULONG64   MmSessionSize;
    ULONG64   MmSystemParentTablePage;

    // Server 2003 addition

    ULONG64   MmVirtualTranslationBase;

    USHORT    OffsetKThreadNextProcessor;
    USHORT    OffsetKThreadTeb;
    USHORT    OffsetKThreadKernelStack;
    USHORT    OffsetKThreadInitialStack;

    USHORT    OffsetKThreadApcProcess;
    USHORT    OffsetKThreadState;
    USHORT    OffsetKThreadBStore;
    USHORT    OffsetKThreadBStoreLimit;

    USHORT    SizeEProcess;
    USHORT    OffsetEprocessPeb;
    USHORT    OffsetEprocessParentCID;
    USHORT    OffsetEprocessDirectoryTableBase;

    USHORT    SizePrcb;
    USHORT    OffsetPrcbDpcRoutine;
    USHORT    OffsetPrcbCurrentThread;
    USHORT    OffsetPrcbMhz;

    USHORT    OffsetPrcbCpuType;
    USHORT    OffsetPrcbVendorString;
    USHORT    OffsetPrcbProcStateContext;
    USHORT    OffsetPrcbNumber;

    USHORT    SizeEThread;

    ULONG64   KdPrintCircularBufferPtr;
    ULONG64   KdPrintBufferSize;

    ULONG64   KeLoaderBlock;

    USHORT    SizePcr;
    USHORT    OffsetPcrSelfPcr;
    USHORT    OffsetPcrCurrentPrcb;
    USHORT    OffsetPcrContainedPrcb;

    USHORT    OffsetPcrInitialBStore;
    USHORT    OffsetPcrBStoreLimit;
    USHORT    OffsetPcrInitialStack;
    USHORT    OffsetPcrStackLimit;

    USHORT    OffsetPrcbPcrPage;
    USHORT    OffsetPrcbProcStateSpecialReg;
    USHORT    GdtR0Code;
    USHORT    GdtR0Data;

    USHORT    GdtR0Pcr;
    USHORT    GdtR3Code;
    USHORT    GdtR3Data;
    USHORT    GdtR3Teb;

    USHORT    GdtLdt;
    USHORT    GdtTss;
    USHORT    Gdt64R3CmCode;
    USHORT    Gdt64R3CmTeb;

    ULONG64   IopNumTriageDumpDataBlocks;
    ULONG64   IopTriageDumpDataBlocks;

    // Longhorn addition

    ULONG64   VfCrashDataBlock;
    ULONG64   MmBadPagesDetected;
    ULONG64   MmZeroedPageSingleBitErrorsDetected;

    // Windows 7 addition

    ULONG64   EtwpDebuggerData;
    USHORT    OffsetPrcbContext;

    // Windows 8 addition

    USHORT    OffsetPrcbMaxBreakpoints;
    USHORT    OffsetPrcbMaxWatchpoints;

    ULONG     OffsetKThreadStackLimit;
    ULONG     OffsetKThreadStackBase;
    ULONG     OffsetKThreadQueueListEntry;
    ULONG     OffsetEThreadIrpList;

    USHORT    OffsetPrcbIdleThread;
    USHORT    OffsetPrcbNormalDpcState;
    USHORT    OffsetPrcbDpcStack;
    USHORT    OffsetPrcbIsrStack;

    USHORT    SizeKDPC_STACK_FRAME;

    // Windows 8.1 Addition

    USHORT    OffsetKPriQueueThreadListHead;
    USHORT    OffsetKThreadWaitReason;

    // Windows 10 RS1 Addition

    USHORT    Padding;
    ULONG64   PteBase;

} KDDEBUGGER_DATA64, *PKDDEBUGGER_DATA64;

#define DUMP_TYPE_FULL 1
#define DMP_HEADER_COMMENT_SIZE             (128)

#ifdef _WIN64

typedef KDDEBUGGER_DATA64 KD_DEBUGGER_DATA_BLOCK;
typedef KD_DEBUGGER_DATA_BLOCK *PKD_DEBUGGER_DATA_BLOCK;

#define DMP_PHYSICAL_MEMORY_BLOCK_SIZE   (700)
#define DMP_CONTEXT_RECORD_SIZE          (3000)
#define DMP_RESERVED_0_SIZE              (4016)

typedef struct _DUMP_HEADER {
    ULONG Signature;
    ULONG ValidDump;
    ULONG MajorVersion;
    ULONG MinorVersion;
    ULONG64 DirectoryTableBase;
    ULONG64 PfnDataBase;
    PLIST_ENTRY PsLoadedModuleList;
    PLIST_ENTRY PsActiveProcessHead;
    ULONG MachineImageType;
    ULONG NumberProcessors;
    ULONG BugCheckCode;
    ULONG64 BugCheckParameter1;
    ULONG64 BugCheckParameter2;
    ULONG64 BugCheckParameter3;
    ULONG64 BugCheckParameter4;
    CHAR VersionUser[32];
    ULONG64  KdDebuggerDataBlock;
    
    union {
        PHYSICAL_MEMORY_DESCRIPTOR PhysicalMemoryBlock;
        UCHAR PhysicalMemoryBlockBuffer [DMP_PHYSICAL_MEMORY_BLOCK_SIZE];
    };

    UCHAR ContextRecord [DMP_CONTEXT_RECORD_SIZE];
    EXCEPTION_RECORD64 Exception;
    ULONG DumpType;
    LARGE_INTEGER RequiredDumpSpace;
    LARGE_INTEGER SystemTime;
    CHAR Comment [DMP_HEADER_COMMENT_SIZE];
    LARGE_INTEGER SystemUpTime;
    ULONG MiniDumpFields;
    ULONG SecondaryDataState;
    ULONG ProductType;
    ULONG SuiteMask;
    ULONG WriterStatus;
    UCHAR Unused1;
    UCHAR KdSecondaryVersion;
    UCHAR Unused[2];
    UCHAR Reserved0[DMP_RESERVED_0_SIZE];
} DUMP_HEADER, *PDUMP_HEADER;

#else

typedef KDDEBUGGER_DATA32 KD_DEBUGGER_DATA_BLOCK;
typedef KD_DEBUGGER_DATA_BLOCK *PKD_DEBUGGER_DATA_BLOCK;

#define DMP_PHYSICAL_MEMORY_BLOCK_SIZE   (700)
#define DMP_CONTEXT_RECORD_SIZE          (1200)
#define DMP_RESERVED_0_SIZE              (1768)
#define DMP_RESERVED_2_SIZE              (16)
#define DMP_RESERVED_3_SIZE              (56)

typedef struct _PHYSICAL_MEMORY_RUN {
    ULONG BasePage;
    ULONG PageCount;
} PHYSICAL_MEMORY_RUN, *PPHYSICAL_MEMORY_RUN;

typedef struct _PHYSICAL_MEMORY_DESCRIPTOR {
    ULONG NumberOfRuns;
    ULONG NumberOfPages;
    PHYSICAL_MEMORY_RUN Run[1];
} PHYSICAL_MEMORY_DESCRIPTOR, *PPHYSICAL_MEMORY_DESCRIPTOR;

typedef struct _DUMP_HEADER {
    ULONG Signature;
    ULONG ValidDump;
    ULONG MajorVersion;
    ULONG MinorVersion;
    ULONG DirectoryTableBase;
    ULONG PfnDataBase;
    PLIST_ENTRY PsLoadedModuleList;
    PLIST_ENTRY PsActiveProcessHead;
    ULONG MachineImageType;
    ULONG NumberOfProcessors;
    ULONG BugCheckCode;
    ULONG BugCheckParameter1;
    ULONG BugCheckParameter2;
    ULONG BugCheckParameter3;
    ULONG BugCheckParameter4;
    UCHAR VersionUser[32];
    UCHAR PaeEnabled;
    UCHAR KdSecondaryVersion;
    UCHAR Spare3[2];
    PVOID KdDebuggerDataBlock;

    union {
        PHYSICAL_MEMORY_DESCRIPTOR PhysicalMemoryBlock;
        UCHAR PhysicalMemoryBlockBuffer[DMP_PHYSICAL_MEMORY_BLOCK_SIZE];
    };
    union {
        CONTEXT Context;
        UCHAR ContextRecord[DMP_CONTEXT_RECORD_SIZE];
    };

    EXCEPTION_RECORD32 Exception;
    UCHAR Comment[DMP_HEADER_COMMENT_SIZE];
    UCHAR Reserved0[DMP_RESERVED_0_SIZE];
    ULONG DumpType;
    ULONG MiniDumpFields;
    ULONG SecondaryDataState;
    ULONG ProductType;
    ULONG SuiteMask;
    ULONG WriterStatus;
    LARGE_INTEGER RequiredDumpSpace;
    UCHAR Reserved2[DMP_RESERVED_2_SIZE];
    LARGE_INTEGER SystemUpTime;
    LARGE_INTEGER SystemTime;
    UCHAR Reserved3[DMP_RESERVED_3_SIZE];
} DUMP_HEADER, *PDUMP_HEADER;

#endif

#pragma warning(default:4201 4214)

NTSTATUS
OsGetDebuggerDataBlock(
    _Out_ PKD_DEBUGGER_DATA_BLOCK* DebuggerData
);

#pragma pack(push, 1)
typedef struct _OS_VERSION
{
    union
    {
        UINT64 raw;

        struct
        {
            UINT8  major;
            UINT8  minor;
            UINT16 servicePack;
            UINT32 buildNumber;
        } f;
    } u;
} OS_VERSION, *POS_VERSION;
#pragma pack(pop)

NTSTATUS
OsVersion(
    _Out_ POS_VERSION osVersion
);


#endif
