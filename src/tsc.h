#ifndef __TSC_H__
#define __TSC_H__

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

typedef struct _TSC_HIT {
	UINT64 Address;
	UINT64 TimeStamp;
    UINT64 Count;
} TSC_HIT, *PTSC_HIT;

typedef struct _TSC_ENTRY {
	TSC_HIT Before;
	TSC_HIT After;
} TSC_ENTRY, *PTSC_ENTRY;

VOID
RdtscEmulate(
    _In_ PLOCAL_CONTEXT Local,
    _In_ PREGISTERS     Regs
);

VOID
RdtscpEmulate(
    _In_ PLOCAL_CONTEXT Local,
    _In_ PREGISTERS     Regs
);

VOID
EnableUserTimeStamp(
    VOID
);


VOID
DisableUserTimeStamp(
    VOID
);


// TODO: export function only for testing purposes
// #ifdef DEBUG
BOOLEAN IsTimmingAttack(
    _In_ PTSC_ENTRY     Sibling
);
// #endif
#endif
