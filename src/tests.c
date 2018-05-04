/*++

Copyright (c) ByteHeed. All Rights Reserved.

Module Name:

    tests.c

Abstract:

    This file implements the test cases.

Environment:

--*/

#include "dsdef.h"

#if defined(WPP_EVENT_TRACING)
#include "tests.tmh"
#endif

#include "tsc.h"
#include "context.h"

#pragma intrinsic(__rdtsc)

PLOCAL_CONTEXT
CreateLocalContext(
    VOID
    );

VOID
DestroyLocalContext(
    _Inout_ PLOCAL_CONTEXT Context
    );

TestResult
TestBasicTimeStampDetectionReuse(
    VOID
    );

TestResult
TestBasicTimeStampDetectionWithSkip(
    VOID
    );

TestResult
TestBasicTimeStampDetection(
    VOID
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, CreateLocalContext)
#pragma alloc_text(PAGE, DestroyLocalContext)
#pragma alloc_text(PAGE, TestBasicTimeStampDetectionReuse)
#pragma alloc_text(PAGE, TestBasicTimeStampDetectionWithSkip)
#pragma alloc_text(PAGE, TestBasicTimeStampDetection)
#pragma alloc_text(PAGE, DsCtlTestRdtscDetection)
#endif

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

    return TimeStampA.QuadPart * (~TimeStampB.QuadPart);
}

ULONG_PTR 
CreateCR3 (
    VOID
    )
{
    return (ULONG_PTR)RandomInt() & 0x00000000FFFFFF00;
}

PLOCAL_CONTEXT 
CreateLocalContext(
    VOID
    )
{
    PLOCAL_CONTEXT Context = NULL;

    PAGED_CODE();

    Context = ExAllocatePoolWithTag( NonPagedPool, 
                                     sizeof( LOCAL_CONTEXT ),
                                     LOCAL_CONTEXT_TAG );

    if (!Context) {
        return NULL;
    }

    RtlZeroMemory( Context, sizeof( LOCAL_CONTEXT ) );

    Context->TscHits = ExAllocatePoolWithTag( NonPagedPool, 
                                              sizeof( TSC_ENTRY ) * MAX_TSC_HITS,
                                              TSC_HITS_TAG);


    if (!Context->TscHits) {
        ExFreePoolWithTag( Context, LOCAL_CONTEXT_TAG );
        return NULL;
    }

    RtlZeroMemory( Context->TscHits, sizeof( TSC_ENTRY ) * MAX_TSC_HITS );
    Context->TscOffset = 0;

    return Context;
}

VOID
DestroyLocalContext(
    _Inout_ PLOCAL_CONTEXT Context
    )
{
    NT_ASSERT( Context );

    PAGED_CODE();

    if (Context ){
        if (Context->TscHits) {
            ExFreePoolWithTag( Context->TscHits, TSC_HITS_TAG );
        }

        ExFreePoolWithTag( Context, LOCAL_CONTEXT_TAG );
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

BOOLEAN
RdtscEmulateTester(
    _In_ PLOCAL_CONTEXT Local,
    _In_ PREGISTERS     Regs,
    _In_ UINT_PTR       Process
)
{
    ULARGE_INTEGER TimeStamp = { 0 };

    // 
    // Simulates a MSR read of IA32_TSC
    //
    TimeStamp.QuadPart = TestReadMsr(IA32_TSC);

    Regs->rdx = TimeStamp.HighPart;
    Regs->rax = TimeStamp.LowPart;

    return ProcessTscEvent(Local->TscHits, Regs->rip, Process, TimeStamp);
}

#define CONSTANT_TSC 0x600

VOID 
InitGlobalTsc(
    VOID
) 
{
    gCurrentTsc = __rdtsc();
}

 
VOID 
AddGlobalTsc(
    UINT64 Value
) 
{
    gCurrentTsc += Value;
}

VOID 
AddTimeStampHit(
    _In_     PLOCAL_CONTEXT Context, 
    _In_opt_ UINT_PTR Process, 
    _In_     UINT_PTR Address, 
    _In_opt_ UINT64 Delta
) {

    REGISTERS      Regs          = { 0 };

    if ( Process == 0 ) {
        Process = CreateCR3();
    }

    Regs.rip = Address;

    AddGlobalTsc(Delta);

    RdtscEmulateTester(Context, &Regs, Process);

}

/*
VOID FillWithOrphans(PLOCAL_CONTEXT Context) {
    NT_ASSERT(Context != NULL);

    UINT_PTR       Process       = 0;
    INT            i             = 0;

    for ( i = 0; i < MAX_TSC_HITS; i++ ) {
        //
        // Unique CR3 per Sibling
        //
        Process = CreateCR3();

        AddTimeStampHit(Context, Process, 0x7F0093E0, CONSTANT_TSC);
    }
}
*/

VOID FillWithSiblings(PLOCAL_CONTEXT Context) {
    UINT_PTR       Process       = 0;
    INT            i             = 0;

    NT_ASSERT(Context != NULL);

    for ( i = 0; i < MAX_TSC_HITS; i++ ) {
        //
        // Unique CR3 per Sibling
        //
        Process = CreateCR3();

        AddTimeStampHit(Context, Process, 0x7F0093E0 | ( i << 16 ), CONSTANT_TSC);
        AddTimeStampHit(Context, Process, 0x7F0093E6 | ( i << 16 ), CONSTANT_TSC);
    }
}

//
// Test to cover situations where siblings are discarded
//
TestResult
TestBasicTimeStampDetectionReuse(
    VOID
    )
{
    PLOCAL_CONTEXT Context       = NULL;
    PTSC_ENTRY     TscHits       = NULL;
    UINT_PTR       Process       = 0;

    INT            i             = 0;

    PAGED_CODE();

    Context = CreateLocalContext();

    if (!Context) {
        return TestErrorNoMemory;
    }

    InitGlobalTsc();


    FillWithSiblings(Context);

    //
    // Verify that all entries are inserted in 
    // different slots
    //

    TscHits = (PTSC_ENTRY) Context->TscHits;

    for ( i = 0; i < MAX_TSC_HITS; i++ ) {
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

    Process = CreateCR3();

    //
    // Let's add new entry so we force reuse
    //
    AddTimeStampHit(Context, Process, 0x07FF6AEE0, 0x600);

    TscHits = (PTSC_ENTRY) Context->TscHits;

    for ( i = 0; i < MAX_TSC_HITS; i++ ) {
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
    INT            i             = 0;

    PAGED_CODE();

    Context = CreateLocalContext();

    if (!Context) {
        return TestErrorNoMemory;
    }

    gCurrentTsc = __rdtsc();

    Process = CreateCR3();

    #define TOTAL_TEST_HITS 256

    for ( i = 0; i < TOTAL_TEST_HITS; i++ ) {
        if ( i > 0 && i % 30 == 0) {
            // Simulates a timming difference affected by a flush
            Addition = (CONSTANT_TSC * 9) + (__rdtsc() & 0xFF);
        } else {
            // Simulates around ~1500-2000 cycles
            Addition = CONSTANT_TSC + (__rdtsc() & 0xFF);
        }

        AddGlobalTsc(Addition);

        Regs.rip = 0x07FF6AEE0;

        RdtscEmulateTester(Context, &Regs, Process);

        // this should be reached each total/30 times
        if ( i > 0 && i % 30 == 0) {
            // Simulates a timming difference affected by a flush
            Addition = (CONSTANT_TSC * 9) + (__rdtsc() & 0xFF);
        } else {
            // Simulates around ~1500-2000 cycles
            Addition = CONSTANT_TSC + (__rdtsc() & 0xFF);
        }

        AddGlobalTsc(Addition);

        Regs.rip = 0x07FF6AEE6;
        RdtscEmulateTester(Context, &Regs, Process);
    }

    NT_ASSERT(Context != NULL);

    TscHits = (PTSC_ENTRY) Context->TscHits;

    for ( i = 0; i < MAX_TSC_HITS; i++ ) {
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
// Test that ensures that difference of addresses is absolute
// instead of masked.
//
TestResult
TestRdtscInstructionBoundaries(
    VOID
)
{
    PLOCAL_CONTEXT Context       = NULL;
    PTSC_ENTRY     TscHits       = NULL;

    REGISTERS      Regs            = { 0 };
    ULONG_PTR      Process         =   0;
    INT            i               =   0;


    Context = CreateLocalContext();

    if (!Context) {
        return TestErrorNoMemory;
    }

    Process = CreateCR3();

    for ( i = 0; i < 256; i++ ) {
        AddGlobalTsc(300);
        Regs.rip = 0x07F7FFFFE;
        RdtscEmulateTester(Context, &Regs, Process);

        AddGlobalTsc(300);
        Regs.rip = 0x07F800003;
        RdtscEmulateTester(Context, &Regs, Process);
    }

    NT_ASSERT(Context != NULL);

    TscHits = (PTSC_ENTRY) Context->TscHits;

    for ( i = 0; i < MAX_TSC_HITS; i++ ) {
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

    REGISTERS      Regs            = { 0 };
    LARGE_INTEGER  RandomAddress   = { 0 };
    ULONG_PTR      Process         =   0;
    INT            i               =   0;
    
    PAGED_CODE();

    Context = CreateLocalContext();

    if (!Context) {
        return TestErrorNoMemory;
    }

    Process = CreateCR3();

    //
    // This test aims to simulate a situation where you have 
    // many different addresses but two of them (siblings)
    // are consecuently being added.
    // 
    for ( i = 0; i < 256 * 6; i++ ) {
        if ( i % 6 == 0 ) {

            AddGlobalTsc(300);
            Regs.rip = 0x07FF6AEE0;
            RdtscEmulateTester(Context, &Regs, Process);

            AddGlobalTsc(300);
            Regs.rip = 0x07FF6AEE6;
            RdtscEmulateTester(Context, &Regs, Process);
        }

        RandomAddress.QuadPart = __rdtsc();
        Regs.rip = (0x7FF70000 | (RandomAddress.LowPart & 0xFFFF));

        AddGlobalTsc(RandomAddress.LowPart & 0xFFFF);
        RdtscEmulateTester(Context, &Regs, CreateCR3());
    }

    NT_ASSERT(Context != NULL);

    TscHits = (PTSC_ENTRY) Context->TscHits;

    for ( i = 0; i < MAX_TSC_HITS; i++ ) {
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

    case TestInstructionBoundaries:
        Request->Result = TestRdtscInstructionBoundaries();
        break;
    default:
        Request->Result = TestErrorRequestInvalid;
        break;
    }
        
    Irp->IoStatus.Information = sizeof( TEST_RDTSC_DETECTION );
    return Status;
}

