#include "wdk7.h"
#include "smp.h" 
#include "sync.h"

typedef struct _PROCESSOR_INFO
{
    volatile LONG      sync;
    PROCESSOR_CALLBACK callback;
    PVOID              context;
    NTSTATUS           error;
} PROCESSOR_INFO, *PPROCESSOR_INFO;

KDEFERRED_ROUTINE DpcRoutine;

VOID
DpcRoutine(
    _In_     PKDPC dpc,
    _In_opt_ PVOID context,
    _In_opt_ PVOID arg1,
    _In_opt_ PVOID arg2
)
{
    PPROCESSOR_INFO info = (PPROCESSOR_INFO)context;
    NTSTATUS status;

    UNREFERENCED_PARAMETER( dpc );
    UNREFERENCED_PARAMETER( arg1 );
    UNREFERENCED_PARAMETER( arg2 );

    if (!ARGUMENT_PRESENT(context)) {
        return;
    }

    status = info->callback(SmpGetCurrentProcessor(), info->context);

    if (status != STATUS_SUCCESS)
    {
        InterlockedCompareExchange(&info->error, status, STATUS_SUCCESS);
    }

    InterlockedIncrement(&info->sync);
}

NTSTATUS
SmpExecuteOnAllProcessors(
    _In_ PROCESSOR_CALLBACK callback,
    _In_opt_ PVOID context
)
{
    NTSTATUS status;
    KIRQL    savedIrql;
    UINT32   ProcessorCount;
    
    ProcessorCount = SmpActiveProcessorCount();

    if ( ProcessorCount > 1)
    {
        KDPC           dpcTraps[MAXIMUM_PROCESSORS] = { 0 };
        PROCESSOR_INFO info                         = { 0 };
        UINT32         i;

        info.callback = callback;
        info.context  = context;
        info.error    = STATUS_SUCCESS;

        KeRaiseIrql(DISPATCH_LEVEL, &savedIrql);

        for (i = 0; i < ProcessorCount; i++)
        {
            if (i != SmpGetCurrentProcessor())
            {
                KeInitializeDpc(&dpcTraps[i], DpcRoutine, &info);
                KeSetTargetProcessorDpc(&dpcTraps[i], (CCHAR)i);
                KeInsertQueueDpc(&dpcTraps[i], &info, 0);
            }
        }

        DpcRoutine(0, &info, 0, 0);

        while (InterlockedCompareExchange(&info.sync, 0, ProcessorCount )) {
            YieldProcessor();
        }

        KeLowerIrql( savedIrql );

        status = info.error;
    }
    else
    {
        KeRaiseIrql(DISPATCH_LEVEL, &savedIrql);

        status = callback(SmpGetCurrentProcessor(), context);

        KeLowerIrql(savedIrql);    
    }

    return status;
}

UINT32
SmpActiveProcessorCount(
    VOID
    )
{
    static UINT32 BitCount = 0;
    UINT32 Shift = 0;
    KAFFINITY ActiveMap;

    if (BitCount == 0) {

        ActiveMap = KeQueryActiveProcessors();

        for (; Shift < sizeof( KAFFINITY ) * 8; Shift++) {

            if (ActiveMap & (1ULL << Shift) ) {
                BitCount++;
            }
        }
    }

    return BitCount;
}

UINT32 
SmpGetCurrentProcessor(
    VOID
    )
{
    return (UINT32)KeGetCurrentProcessorNumber();
}
