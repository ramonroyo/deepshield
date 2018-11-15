#ifndef __TSC_H__
#define __TSC_H__

#include "dsdef.h"
#include "hvm.h"
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
BOOLEAN TdIsTimmingCandidate(
    _In_ PTSC_ENTRY     Sibling
);

BOOLEAN TdProcessTscEvent(
    _In_ PTSC_ENTRY     Head,
    _In_ UINTN          OffensiveAddress,
    _In_ ULARGE_INTEGER TimeStamp
);
// #endif
#endif
