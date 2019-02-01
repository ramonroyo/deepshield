#ifndef __TSC_H__
#define __TSC_H__

#include "context.h"

//
//  0F 31 (RDTSCP) | 64 / 32-bit
//
#define RDTSC_INS_SIZE 2

//
//  0F 01 F9 (RDTSCP) | 64 / 32-bit
//
#define RDTSCP_INS_SIZE 3

#define MAX_TSC_HITS 100

#define TSC_HASH_BITS_HIGH sizeof(UINTN) * 4

#define SIBLING_DISTANCE_LIMIT     0xFF

typedef struct _TSC_HIT {
    //
    // [SG] REVIEW: Are we able to change Address to UINTN?
    //
    UINTN Address;
    UINT64 TimeStamp;
    UINT64 Count;
} TSC_HIT, *PTSC_HIT;

typedef struct _TSC_ENTRY {
    TSC_HIT  Before;
    TSC_HIT  After;
    ULONG    Skips;
    UINT64   Difference;
    UINTN    TscHash;
} TSC_ENTRY, *PTSC_ENTRY;


#define TD_MEMORY_REFERENCES  0x00000001
#define TD_INSTRUCTIONS_INVALID 0x00000002

typedef struct _TSC_REPORT {
    UINTN      ProcessId;
    UINTN      ThreadId;
    PTSC_ENTRY Tsc;
    UINT8      Code[SIBLING_DISTANCE_LIMIT];
    UINTN      CodeSize;
    ULONG      Flags;
} TSC_REPORT, *PTSC_REPORT;

VOID
VmRdtscEmulate(
    _In_ PVCPU_CONTEXT Local,
    _In_ PGP_REGISTERS Registers
);

VOID
VmRdtscpEmulate(
    _In_ PVCPU_CONTEXT Local,
    _In_ PGP_REGISTERS Registers
);

// TODO: export function only for testing purposes
// #ifdef DEBUG
BOOLEAN TdIsTimingCandidate(
    _In_ PTSC_ENTRY     Sibling
);

BOOLEAN TdProcessTscEvent(
    _In_ PTSC_ENTRY     Head,
    _In_ UINTN          OffensiveAddress,
    _In_ ULARGE_INTEGER TimeStamp
);
// #endif
#endif
