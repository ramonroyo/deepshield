#include "context.h"
#include "mem.h"
#include "mmu.h"
#include "smp.h"

PGLOBAL_CONTEXT gGlobalContext = 0;
PLOCAL_CONTEXT  gLocalContexts = 0;

BOOLEAN
GlobalContextConfigure(
    _In_ PGLOBAL_CONTEXT global
)
{
    memset(global, 0, sizeof(GLOBAL_CONTEXT));

    //
    // msr
    //
    global->msrBitmap = MemAllocAligned(PAGE_SIZE, PAGE_SIZE);
    if (global->msrBitmap == NULL)
        return FALSE;
    memset(global->msrBitmap, 0, PAGE_SIZE);

    return TRUE;
}

VOID
GlobalContextReset(
    _In_ PGLOBAL_CONTEXT global
)
{
    if(global)
    {
        if (global->msrBitmap)
            MemFree(global->msrBitmap);

        memset(global, 0, sizeof(GLOBAL_CONTEXT));
    }
}

BOOLEAN
LocalContextConfigure(
    _In_ PLOCAL_CONTEXT local
)
{
    memset(local, 0, sizeof(LOCAL_CONTEXT));

    return TRUE;
}

VOID
LocalContextReset(
    _In_ PLOCAL_CONTEXT local
)
{
    if (local)
    {
        memset(local, 0, sizeof(LOCAL_CONTEXT));
    }
}


PGLOBAL_CONTEXT
GlobalCtx(
    VOID
)
{
    return gGlobalContext;
}

PLOCAL_CONTEXT
LocalCtx(
	VOID
)
{
	return &gLocalContexts[SmpCurrentCore()];
}

PLOCAL_CONTEXT
LocalCtxForCore(
    _In_ UINT32 core
)
{
    return &gLocalContexts[core];
}

PREGISTERS
LocalRegs(
    VOID
)
{
    return HvmCoreRegisters(HvmCoreCurrent());
}
