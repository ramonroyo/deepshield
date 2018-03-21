#ifndef __TSC_H__
#define __TSC_H__

#include "hvm.h"
#include "context.h"

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

#endif
