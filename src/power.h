#ifndef _POWER_
#define _POWER_

#include <ntifs.h>
#include "wdk7.h"

#pragma warning(disable:4201)   // nameless struct/union

typedef CALLBACK_FUNCTION POWER_CALLBACK_FUNCTION;
typedef POWER_CALLBACK_FUNCTION *PPOWER_CALLBACK_FUNCTION;

_IRQL_requires_max_( APC_LEVEL )
_IRQL_requires_same_
PVOID
DsRegisterPowerChangeCallback(
    _In_ PPOWER_CALLBACK_FUNCTION CallbackFunction,
    _In_opt_ PVOID CallbackContext,
    _In_ ULONG Flags
    );

_IRQL_requires_max_( APC_LEVEL )
_IRQL_requires_same_
VOID
DsDeregisterPowerChangeCallback(
    _In_ PVOID CallbackHandle
    );

_IRQL_requires_same_
_Function_class_( CALLBACK_FUNCTION )
VOID
DsPowerChangeCallback(
    _In_ PVOID CallbackContext,
    _In_ PVOID PowerStateEvent,
    _In_ PVOID EventSpecifics
    );


#endif