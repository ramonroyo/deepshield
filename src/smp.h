/**
*  @file    smp.h
*  @brief   SMP related functions
*/
#ifndef __SMP_H__
#define __SMP_H__

#include <ntifs.h>

/**
*  Callback type to be executed on every Vcpu.
*
* @param VcpuId   [in]     Vcpu logical index where the code is to be executed.
* @param context  [in opt] Context of the function.
* @return Status codes.
*/
typedef NTSTATUS (__stdcall * PROCESSOR_CALLBACK)(
    _In_     UINT32 VcpuId,
    _In_opt_ PVOID  context
    );

/**
*  Executes a piece of code in all processors by sending an IPI.
*
* @param callback [in]     Function to execute on every logical processor.
* @param context  [in opt] Context of the function.
* @return Success if every function succeeded, first error encountered otherwise.
*/
NTSTATUS
SmpRunPerProcessor(
    _In_     PROCESSOR_CALLBACK callback, 
    _In_opt_ PVOID              context
);

/**
* Indicates the total number of logical processors in the system.
*
* @return The number of logical processors.
*/
UINT32
SmpActiveProcessorCount(
    VOID
);

/**
* Indicates the index of the logical processor executing this function.
* The index is zero-based
*
* @return The current logical processor index.
*/
UINT32 
SmpGetCurrentProcessor(
    VOID
);

#endif
