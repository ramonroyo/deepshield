/**
*  @file    smp.h
*  @brief   SMP related functions
*/
#ifndef __SMP_H__
#define __SMP_H__

#include <ntddk.h>

/**
*  Callback type to be executed on every core.
*
* @param core     [in]     Core logical index where the code is to be executed.
* @param context  [in opt] Context of the function.
* @return Status codes.
*/
typedef NTSTATUS (__stdcall * PROCESSOR_CALLBACK)(
    _In_     UINT32 core,
    _In_opt_ PVOID  context
    );

/**
*  Executes a piece of code in all cores by sending an IPI.
*
* @param callback [in]     Function to execute on every core.
* @param context  [in opt] Context of the function.
* @return Success if every function succeeded, first error encountered otherwise.
*/
NTSTATUS
SmpExecuteOnAllCores(
    _In_     PROCESSOR_CALLBACK callback, 
    _In_opt_ PVOID              context
);

/**
* Indicates the total number of logical cores in the system.
*
* @return The number of logical cores.
*/
UINT32
SmpNumberOfCores(
    VOID
);

/**
* Indicates the index of the logical core executing this function.
* The index is zero-based [0 - OsNumberOfCores() - 1]
*
* @return The current logical core index.
*/
UINT32 
SmpCurrentCore(
    VOID
);

#endif
