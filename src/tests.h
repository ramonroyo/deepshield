/*++

Copyright (C) ByteHeed. All Rights Reserved

Module Name:

    tests.h

--*/

#ifndef _TESTS_H_
#define _TESTS_H_

#include "dsdef.h"

typedef enum EnumTestResult {
    TestSuccess = 0,
    TestErrorNoMemory,
    TestErrorRequestInvalid,
    TestErrorDetectionFailed,
    TestErrorReuse,
    TestErrorSkipping,
    TestErrorDifference,
    TestErrorUnknown
} TestResult;

typedef enum EnumTestRequest {
    TestBasicRdtscDetection = 1,
    TestBasicRdtscDetectionWithSkip,
    TestRdtscDetectionReuse,
    TestRdtscDetection,
    TestInstructionBoundaries,
    TestUnknown
} TestRequest;

typedef struct _TEST_RDTSC_DETECTION {
    TestRequest Request;
    TestResult  Result;
} TEST_RDTSC_DETECTION, *PTEST_RDTSC_DETECTION;

NTSTATUS 
DsCtlTestRdtscDetection(
    _In_ PIRP Irp,
    _In_ PIO_STACK_LOCATION IrpStack
    );

#endif
