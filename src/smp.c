#include "smp.h" 
#include "sync.h"

typedef struct _PROCESSOR_INFO
{
    volatile LONG      sync;
    PROCESSOR_CALLBACK callback;
    PVOID              context;
    NTSTATUS           error;
} PROCESSOR_INFO, *PPROCESSOR_INFO;

VOID
DpcRoutine(
    PKDPC dpc,
    PVOID context,
    PVOID arg1,
    PVOID arg2
)
{
    PPROCESSOR_INFO info = (PPROCESSOR_INFO)context;
    NTSTATUS status;

    UNREFERENCED_PARAMETER( dpc );
    UNREFERENCED_PARAMETER( arg1 );
    UNREFERENCED_PARAMETER( arg2 );

    status = info->callback(SmpCurrentCore(), info->context);

    if(status != STATUS_SUCCESS)
    {
        InterlockedCompareExchange(&info->error, status, STATUS_SUCCESS);
    }

    InterlockedIncrement(&info->sync);
}

NTSTATUS
SmpExecuteOnAllCores(
    _In_     PROCESSOR_CALLBACK callback, 
    _In_opt_ PVOID              context
)
{
    NTSTATUS status;
    KIRQL    savedIrql;
    UINT32   numberOfCores;
    
    numberOfCores = SmpNumberOfCores();

    if (numberOfCores > 1)
    {
        KDPC           dpcTraps[MAXIMUM_PROCESSORS] = { 0 };
        PROCESSOR_INFO info                         = { 0 };
        UINT32         i;

        info.callback = callback;
        info.context  = context;
        info.error    = STATUS_SUCCESS;

        KeRaiseIrql(DISPATCH_LEVEL, &savedIrql);

        for (i = 0; i < numberOfCores; i++)
        {
            if (i != SmpCurrentCore())
            {
                KeInitializeDpc(&dpcTraps[i], DpcRoutine, &info);
                KeSetTargetProcessorDpc(&dpcTraps[i], (CCHAR)i);
                KeInsertQueueDpc(&dpcTraps[i], &info, 0);
            }
        }

        DpcRoutine(0, &info, 0, 0);

        KeLowerIrql(savedIrql);

        while (InterlockedCompareExchange(&info.sync, 0, numberOfCores))
            YieldProcessor();

        status = info.error;
    }
    else
    {
        KeRaiseIrql(DISPATCH_LEVEL, &savedIrql);

        status = callback(SmpCurrentCore(), context);

        KeLowerIrql(savedIrql);	
    }

    return status;
}

UINT32
SmpNumberOfCores(
    VOID
)
{
    static UINT32 numberOfCores = (UINT32)-1;

    if(numberOfCores == (UINT32)-1)
        numberOfCores = KeQueryMaximumProcessorCountEx(ALL_PROCESSOR_GROUPS);

    return numberOfCores;
}

UINT32 
SmpCurrentCore(
    VOID
)
{
    return KeGetCurrentProcessorNumberEx(0);
}
