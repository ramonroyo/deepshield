#include "wdk7.h"
#include "context.h"
#include "tsc.h"
#include "mem.h"
#include "mmu.h"
#include "smp.h"

PHVM_CONTEXT gHvmContext = 0;
PVCPU_CONTEXT gVcpuContexts = 0;
extern UINTN gSystemPageDirectoryTable;

BOOLEAN
InitializeHvmContext(
    _In_ PHVM_CONTEXT HvmContext
    )
{
    RtlZeroMemory( HvmContext, sizeof( HVM_CONTEXT ) );
    HvmContext->SystemCr3 = gSystemPageDirectoryTable;

    HvmContext->MsrBitmap = MemAllocAligned( PAGE_SIZE, PAGE_SIZE );
    if (HvmContext->MsrBitmap == NULL) {
        return FALSE;
    }

    RtlZeroMemory( HvmContext->MsrBitmap, PAGE_SIZE );

    return TRUE;
}

VOID
ResetHvmContext(
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
InitializeVcpuContext(
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
ResetVcpuContext(
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
    return gHvmContext;
}

PVCPU_CONTEXT
LocalCtx(
    VOID
    )
{
    return &gVcpuContexts[SmpGetCurrentProcessor()];
}

PVCPU_CONTEXT
LocalCtxForVcpuId(
    _In_ UINT32 VcpuId
    )
{
    return &gVcpuContexts[VcpuId];
}