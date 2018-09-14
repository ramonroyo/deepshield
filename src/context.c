#include "wdk7.h"
#include "context.h"
#include "tsc.h"
#include "mem.h"
#include "mmu.h"
#include "smp.h"

PHVM_CONTEXT gGlobalContext = 0;
PVCPU_CONTEXT gLocalContexts = 0;

BOOLEAN
GlobalContextConfigure(
    _In_ PHVM_CONTEXT HvmContext
)
{
    RtlZeroMemory( HvmContext, sizeof( HVM_CONTEXT ) );
    HvmContext->SystemCr3 = __readcr3();

    HvmContext->MsrBitmap = MemAllocAligned( PAGE_SIZE, PAGE_SIZE );
    if (HvmContext->MsrBitmap == NULL) {
        return FALSE;
    }

    RtlZeroMemory( HvmContext->MsrBitmap, PAGE_SIZE );

    return TRUE;
}

VOID
GlobalContextReset(
    _In_ PHVM_CONTEXT HvmContext
    )
{
    if (HvmContext) {
        if (HvmContext->MsrBitmap) {
            MemFree( HvmContext->MsrBitmap );
        }

        RtlZeroMemory( HvmContext, sizeof(HVM_CONTEXT) );
    }
}

BOOLEAN
LocalContextConfigure(
    _In_ PVCPU_CONTEXT VcpuContext
    )
{
    RtlZeroMemory( VcpuContext, sizeof(VCPU_CONTEXT) );

    VcpuContext->TscHits = MemAlloc(sizeof(TSC_ENTRY) * MAX_TSC_HITS);
    VcpuContext->TscOffset = 0;

    if (VcpuContext->TscHits == NULL) {
        return FALSE;
    }

    RtlZeroMemory(VcpuContext->TscHits, sizeof(TSC_ENTRY) * MAX_TSC_HITS);

    return TRUE;
}

VOID
LocalContextReset(
    _In_ PVCPU_CONTEXT VcpuContext
    )
{
    if (VcpuContext) {
        if (VcpuContext->TscHits) {
            MemFree( VcpuContext->TscHits );
        }

        RtlZeroMemory( VcpuContext, sizeof(VCPU_CONTEXT) );
    }
}

PHVM_CONTEXT
GlobalCtx(
    VOID
    )
{
    return gGlobalContext;
}

PVCPU_CONTEXT
LocalCtx(
    VOID
    )
{
    return &gLocalContexts[SmpGetCurrentProcessor()];
}

PVCPU_CONTEXT
LocalCtxForVcpu(
    _In_ UINT32 VcpuId
    )
{
    return &gLocalContexts[VcpuId];
}