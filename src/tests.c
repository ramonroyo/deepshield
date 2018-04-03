#include <ntddk.h>
#include "tests.h"
#include "tsc.h"
#include "context.h"


#define LOCAL_CONTEXT_TAG 'CLSD'
#define TSC_HITS_TAG      'HTSD'

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

//
// Preliminar dummy test of RDTSC
//
TestResult
TestTimeStampDetection(
    VOID
)
{
    PLOCAL_CONTEXT Context       = NULL;
    PTSC_ENTRY     TscHits       = NULL;
    REGISTERS      Regs          = { 0 };
    LARGE_INTEGER  TimeStamp     = { 0 };
    LARGE_INTEGER  RandomAddress = { 0 };

    Context = CreateLocalContext();

    if (!Context) {
        return TestErrorNoMemory;
    }

    //
    // This test aims to simulate a situation where you have 
    // many different addresses but two of them (siblings)
    // are consecuently being added.
    // 
    for ( int i = 0; i < 256*6; i++ ) {
        if ( i % 6 == 0 ) {
            TimeStamp.QuadPart = __rdtsc();
            Regs.rip = 0x00007FF6AED493E0;
            RdtscEmulate(Context, &Regs);
            Regs.rip = 0x00007FF6AED493E6;
            RdtscEmulate(Context, &Regs);
        }

        RandomAddress.QuadPart = __rdtsc();
        Regs.rip = 0x00007FF6 | RandomAddress.LowPart;
        RdtscEmulate(Context, &Regs);
    }

    NT_ASSERT(Context != NULL);

    TscHits = (PTSC_ENTRY) Context->TscHits;

    for ( int i = 0; i < MAX_TSC_HITS; i++ ) {
        PTSC_ENTRY Entry = &TscHits[i];

        if ( IsTimmingAttack(Entry) ) {
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
    case TestDummyRdtscDetection:
        Request->Result = TestTimeStampDetection();
        break;
    default:
        Request->Result = TestErrorRequestInvalid;
        break;
    }
        
    Irp->IoStatus.Information = sizeof( TEST_RDTSC_DETECTION );
    return Status;
}

