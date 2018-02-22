#ifndef __CONTEXT_H__
#define __CONTEXT_H__

#include "hvm.h"

typedef struct _LOCAL_CONTEXT
{
	UINT_PTR dummy;
} LOCAL_CONTEXT, *PLOCAL_CONTEXT;

typedef struct _GLOBAL_CONTEXT
{
    PUCHAR   msrBitmap;
} GLOBAL_CONTEXT, *PGLOBAL_CONTEXT;

BOOLEAN
GlobalContextConfigure(
    _In_ PGLOBAL_CONTEXT context
);

VOID
GlobalContextReset(
    _In_ PGLOBAL_CONTEXT context
);

BOOLEAN
LocalContextConfigure(
    _In_ PLOCAL_CONTEXT  local
);

VOID
LocalContextReset(
    _In_ PLOCAL_CONTEXT  local
);


PGLOBAL_CONTEXT
GlobalCtx(
    VOID
);

PLOCAL_CONTEXT
LocalCtx(
    VOID
);


PLOCAL_CONTEXT
LocalCtxForCore(
    _In_ UINT32 core
);

PREGISTERS
LocalRegs(
    VOID
);

#endif
