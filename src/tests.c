#include <ntddk.h>
#include "tests.h"
#include "tsc.h"
#include "context.h"


#define LOCAL_CONTEXT_TAG 'CLSD'
#define TSC_HITS_TAG      'HTSD'

UINT64 RandomInt(
    VOID
)
{
    LARGE_INTEGER TimeStampA = { 0 };
    LARGE_INTEGER TimeStampB = { 0 };

    TimeStampA.QuadPart = __rdtsc();
    TimeStampB.QuadPart = __rdtsc();

    return TimeStampA.QuadPart * (!TimeStampB.QuadPart);
}

ULONG_PTR 
CreateCR3 (
    VOID
    )
{
    return (ULONG_PTR)RandomInt() & 0x00000000FFFFFF00;
}


PLOCAL_CONTEXT CreateLocalContext(
    VOID
)
{
    PLOCAL_CONTEXT Context = NULL;

    Context = ExAllocatePoolWithTag(NonPagedPool, sizeof(LOCAL_CONTEXT), LOCAL_CONTEXT_TAG);

    if ( !Context ) {
        return NULL;
    }

    memset(Context, 0, sizeof(LOCAL_CONTEXT));

    Context->TscHits = ExAllocatePoolWithTag(NonPagedPool, sizeof(TSC_ENTRY) * MAX_TSC_HITS, TSC_HITS_TAG);


    if ( !Context->TscHits ) {
        ExFreePoolWithTag(Context, LOCAL_CONTEXT_TAG);
        return NULL;
    }

    memset(Context->TscHits, 0, sizeof(TSC_ENTRY) * MAX_TSC_HITS);

    Context->TscOffset = 0;

    return Context;
}

DestroyLocalContext(
    PLOCAL_CONTEXT Context
)
{
    NT_ASSERT(Context);

    if ( Context ) {
        if (Context->TscHits) {
            ExFreePoolWithTag(Context->TscHits, TSC_HITS_TAG);
        }

        ExFreePoolWithTag(Context, LOCAL_CONTEXT_TAG);
    }
}

UINT64 gCurrentTsc = 0;

UINT64 
TestReadMsr(
    _In_ ULONG Index
) 
{

#ifndef DEBUG
    UNREFERENCED_PARAMETER(Index);
#endif

    ASSERT( Index == IA32_TSC );

    return gCurrentTsc;
}

VOID
RdtscEmulateTester(
    _In_ PLOCAL_CONTEXT Local,
	_In_ PREGISTERS     Regs,
    _In_ UINT_PTR       Process
)
{
    ULARGE_INTEGER TimeStamp = { 0 };
    TimeStamp.QuadPart = TestReadMsr(IA32_TSC);

    Regs->rdx = TimeStamp.HighPart;
    Regs->rax = TimeStamp.LowPart;

    ProcessTscEvent(Local->TscHits, Regs->rip, Process, TimeStamp);

    // Not necessary while testing
    // InstrRipAdvance(Regs);
}

 
VOID 
AddGlobalTsc(
    UINT64 Value
) 
{
    gCurrentTsc += Value;
}

//
// Preliminar dummy test of RDTSC
//
TestResult
TestBasicTimeStampDetectionReuse(
    VOID
)
{
    #define CONSTANT_TSC 0x600

    PLOCAL_CONTEXT Context       = NULL;
    PTSC_ENTRY     TscHits       = NULL;
    UINT_PTR       Process       = 0;

    REGISTERS      Regs          = { 0 };

    Context = CreateLocalContext();

    if (!Context) {
        return TestErrorNoMemory;
    }

    gCurrentTsc = __rdtsc();


    for ( int i = 0; i < MAX_TSC_HITS; i++ ) {
        // create a new CR3 per Sibling
        Process = CreateCR3();

        // Sibling1
        Regs.rip = (UINT_PTR) (0x00007FF6AE0093E0 | ( i << 16 ));
        AddGlobalTsc(CONSTANT_TSC);
        RdtscEmulateTester(Context, &Regs, Process);

        // Sibling2
        Regs.rip = (UINT_PTR) (0x00007FF6AE0093E6 | ( i << 16 ));
        AddGlobalTsc(CONSTANT_TSC);
        RdtscEmulateTester(Context, &Regs, Process);

    }

    // Verify that all entries are inserted in 
    // different entries

    TscHits = (PTSC_ENTRY) Context->TscHits;

    for ( int i = 0; i < MAX_TSC_HITS; i++ ) {
        PTSC_ENTRY Entry = &TscHits[i];

        if ( Entry->Before.Address == 0 ||
             Entry->After.Address  == 0 ) {
            return TestErrorReuse;
        }

        if ( Entry->Skips > 0 ) {
            return TestErrorSkipping;
        }

        if ( Entry->Difference != CONSTANT_TSC ) {
            return TestErrorDifference;
        }
    }

    // Let's force to reuse address
    Process = CreateCR3();
    Regs.rip = (UINT_PTR) 0x00007FF6AED493E0;
    AddGlobalTsc((0x600 + (__rdtsc() & 0xFF)));
    RdtscEmulateTester(Context, &Regs, Process);

    TscHits = (PTSC_ENTRY) Context->TscHits;

    for ( int i = 0; i < MAX_TSC_HITS; i++ ) {
        PTSC_ENTRY Entry = &TscHits[i];

        if ( Entry->Process == Process ) {
            if ( i != 0 ) {
                DestroyLocalContext(Context);
                return TestErrorReuse;
            }
            DestroyLocalContext(Context);
            return TestSuccess;
        }
    }

    DestroyLocalContext(Context);

    return TestErrorDetectionFailed;

}


//
// Preliminar dummy test of RDTSC
//
TestResult
TestBasicTimeStampDetectionWithSkip(
    VOID
)
{
    PLOCAL_CONTEXT Context       = NULL;
    PTSC_ENTRY     TscHits       = NULL;
    UINT_PTR       Process       = 0;

    REGISTERS      Regs          = { 0 };
    UINT32         Addition      = 0;

    Context = CreateLocalContext();

    if (!Context) {
        return TestErrorNoMemory;
    }

    gCurrentTsc = __rdtsc();

    Process = CreateCR3();

    #define TOTAL_TEST_HITS 256 * 6

    for ( int i = 0; i < TOTAL_TEST_HITS; i++ ) {
        if ( i % 6 == 0 ) {
            if ( i % (TOTAL_TEST_HITS/12) == 0) {
                // Simulates a timming difference affected by a flush
                Addition = (CONSTANT_TSC * 9) + (__rdtsc() & 0xFF);
            } else {
                // Simulates around ~1500-2000 cycles
                Addition = CONSTANT_TSC + (__rdtsc() & 0xFF);
            }

            AddGlobalTsc(Addition);

            Regs.rip = 0x00007FF6AED493E0;

            RdtscEmulateTester(Context, &Regs, Process);

            // this should be reached each total/12 times
            if ( i % (TOTAL_TEST_HITS/12) == 0) {
                // Simulates a timming difference affected by a flush
                Addition = (CONSTANT_TSC * 9) + (__rdtsc() & 0xFF);
            } else {
                // Simulates around ~1500-2000 cycles
                Addition = CONSTANT_TSC + (__rdtsc() & 0xFF);
            }

            AddGlobalTsc(Addition);
            Regs.rip = (UINT_PTR) 0x00007FF6AED493E6;

            RdtscEmulateTester(Context, &Regs, Process);
        }

    }

    NT_ASSERT(Context != NULL);

    TscHits = (PTSC_ENTRY) Context->TscHits;

    for ( int i = 0; i < MAX_TSC_HITS; i++ ) {
        PTSC_ENTRY Entry = &TscHits[i];

        if ( IsTimmingAttack(Entry) ) {
            DestroyLocalContext(Context);
            return TestSuccess;
        }
    }

    DestroyLocalContext(Context);

    return TestErrorDetectionFailed;

}


//
// Preliminar dummy test of RDTSC
//
TestResult
TestBasicTimeStampDetection(
    VOID
)
{
    PLOCAL_CONTEXT Context       = NULL;
    PTSC_ENTRY     TscHits       = NULL;

    REGISTERS      Regs          = { 0 };
    LARGE_INTEGER  RandomAddress = { 0 };
    UINT8          FakeMapping[17] = { 0 };

    Context = CreateLocalContext();

    if (!Context) {
        return TestErrorNoMemory;
    }

    //
    // This test aims to simulate a situation where you have 
    // many different addresses but two of them (siblings)
    // are consecuently being added.
    // 
    for ( int i = 0; i < 256 * 6; i++ ) {
        if ( i % 6 == 0 ) {
            Regs.rip = (UINT_PTR) 0x00007FF6AED493E0;
            RdtscEmulate(Context, &Regs, 0, FakeMapping);
            Regs.rip = (UINT_PTR) 0x00007FF6AED493E6;
            RdtscEmulate(Context, &Regs, 0, FakeMapping);
        }

        RandomAddress.QuadPart = __rdtsc();
        Regs.rip = (UINT_PTR) (0x00007FF6 | RandomAddress.LowPart);
    }

    NT_ASSERT(Context != NULL);

    TscHits = (PTSC_ENTRY) Context->TscHits;

    for ( int i = 0; i < MAX_TSC_HITS; i++ ) {
        PTSC_ENTRY Entry = &TscHits[i];

        if ( IsTimmingAttack(Entry) ) {
            DestroyLocalContext(Context);
            return TestSuccess;
        }
    }

    DestroyLocalContext(Context);

    return TestErrorDetectionFailed;

}

NTSTATUS
DsCtlTestRdtscDetection(
    _In_ PIRP Irp,
    _In_ PIO_STACK_LOCATION IrpStack
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PTEST_RDTSC_DETECTION Request;
    ULONG InputLength;
    ULONG OutputLength;

    PAGED_CODE();

    InputLength = IrpStack->Parameters.DeviceIoControl.InputBufferLength;
    OutputLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;

    if (max( InputLength, OutputLength ) < sizeof( TEST_RDTSC_DETECTION )) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    Request = (PTEST_RDTSC_DETECTION) Irp->AssociatedIrp.SystemBuffer;

    switch ( Request->Request ) {
    case TestBasicRdtscDetection:
        Request->Result = TestBasicTimeStampDetection();
        break;
    case TestBasicRdtscDetectionWithSkip:
        Request->Result = TestBasicTimeStampDetectionWithSkip();
        break;
    case TestRdtscDetectionReuse:
        Request->Result = TestBasicTimeStampDetectionReuse();
        break;
    default:
        Request->Result = TestErrorRequestInvalid;
        break;
    }
        
    Irp->IoStatus.Information = sizeof( TEST_RDTSC_DETECTION );
    return Status;
}

