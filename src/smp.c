#include "smp.h"
#include "mem.h"

typedef struct _IPI_ARGUMENT
{
    UINT32             used;
    PROCESSOR_CALLBACK callback;
    PVOID              context;
    NTSTATUS           result;
} IPI_ARGUMENT, *PIPI_ARGUMENT;

ULONG_PTR 
SmppIpiExecution(
    _In_ ULONG_PTR argument
)
{
    PIPI_ARGUMENT arg;
    PIPI_ARGUMENT slot;

    arg  = (PIPI_ARGUMENT)argument;
    slot = arg + SmpCurrentCore();

    slot->used = 1;
    slot->result = slot->callback(SmpCurrentCore(), slot->context);

    return 0;
}

NTSTATUS 
SmpExecuteOnAllCores(
    _In_     PROCESSOR_CALLBACK callback, 
    _In_opt_ PVOID              context
)
{
    NTSTATUS      status = STATUS_SUCCESS;
    PIPI_ARGUMENT arg;
    UINT32        nc;
    UINT32        i;
    
    if (!callback)
        return STATUS_INVALID_PARAMETER;

    nc = SmpNumberOfCores();
    if (nc > 1)
    {
        //
        // Scatter
        //
        arg = ExAllocatePoolWithTag(NonPagedPool, sizeof(IPI_ARGUMENT) * nc, 'ppms');
        if (!arg)
            return STATUS_INSUFFICIENT_RESOURCES;

        for (i = 0; i < nc; i++)
        {
            arg[i].used     = 0;
            arg[i].callback = callback;
            arg[i].context  = context;
            arg[i].result   = 0;
        }

        //
        // Multicore Call
        //
        KeIpiGenericCall(SmppIpiExecution, (ULONG_PTR)arg);

        //
        // Gather
        //
        for (i = 0; i < nc; i++)
        {
            if (arg[i].used)
            {
                if (!NT_SUCCESS(arg[i].result))
                {
                    status = arg[i].result;
                    break;
                }
            }
        }

        ExFreePool(arg);
    }
    else
    {
        KIRQL oldIrql = 0;
        KeRaiseIrql(IPI_LEVEL, &oldIrql);

        status = callback(SmpCurrentCore(), context);

        KeLowerIrql(oldIrql);
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
