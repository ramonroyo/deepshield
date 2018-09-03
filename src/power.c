/*++

Copyright (c) ByteHeed. All Rights Reserved.

Module Name:

    power.c

Abstract:

    This file implements the power state change callback and registration
    routines.

Environment:

--*/

#include "dsdef.h"

#if defined(WPP_EVENT_TRACING)
#include "power.tmh"
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, DsRegisterPowerChangeCallback)
#pragma alloc_text(PAGE, DsDeregisterPowerChangeCallback)
#pragma alloc_text(PAGE, DsPowerChangeCallback)
#endif

_IRQL_requires_same_
_Function_class_(CALLBACK_FUNCTION)
VOID
DsPowerChangeCallback(
    _In_ PVOID CallbackContext,
    _In_ PVOID PowerStateEvent,
    _In_ PVOID EventSpecifics
    )
{
    NTSTATUS Status;

    UNREFERENCED_PARAMETER( CallbackContext );

    //
    //  This is invoked before the power manager sends the system set-power IRP
    //  so the system power state transition has not been commited yet.
    //
    if (PowerStateEvent != (PVOID) PO_CB_SYSTEM_STATE_LOCK) {
        return;
    }

    //
    //  A transition to a system sleep or shutdown state is imminent. 
    //
    if (EventSpecifics == (PVOID) 1 ) {

        //
        //  We have reentered S0 (note that we may have never left S0).
        //  Load the hypervisor.
        //
        if (FALSE == DsIsShieldRunning() 
            && FlagOn ( gStateFlags, DSH_GFL_SHIELD_SUSPENDED )) {

            NT_ASSERT( !FlagOn( gStateFlags, DSH_GFL_SHIELD_STARTED ) );
            ClearFlag( gStateFlags, DSH_GFL_SHIELD_SUSPENDED );

            Status = DsStartShield();

            if (NT_SUCCESS( Status )) {
                SetFlag( gStateFlags, DSH_GFL_SHIELD_STARTED );
            }
        }
    }
    else if (EventSpecifics == (PVOID) 0) {

        //
        //  Leaving the ON state (S0) to sleep (S1/S2/S3) or hibernate (S4).
        //  Unload the hypervisor.
        //
        if (DsIsShieldRunning()) {
            NT_ASSERT( FlagOn( gStateFlags, DSH_GFL_SHIELD_STARTED ) );

            Status = DsStopShield();

            if (NT_SUCCESS( Status) ) {
                ClearFlag( gStateFlags, DSH_GFL_SHIELD_STARTED );
                SetFlag( gStateFlags, DSH_GFL_SHIELD_SUSPENDED );
            }
        }
    }
}

_IRQL_requires_max_( APC_LEVEL )
_IRQL_requires_same_
PVOID
DsRegisterPowerChangeCallback(
    _In_ PPOWER_CALLBACK_FUNCTION CallbackFunction,
    _In_opt_ PVOID CallbackContext,
    _In_ ULONG Flags
    )
{
    UNICODE_STRING PowerName;
    PCALLBACK_OBJECT CallbackObject;
    PVOID CallbackHandle = NULL;
    OBJECT_ATTRIBUTES Oa;

    UNREFERENCED_PARAMETER( Flags );

    RtlInitUnicodeString( &PowerName, L"\\Callback\\PowerState" );
    InitializeObjectAttributes( &Oa,
                                &PowerName,
                                OBJ_CASE_INSENSITIVE | OBJ_PERMANENT,
                                NULL,
                                NULL);

    if (NT_SUCCESS( ExCreateCallback( &CallbackObject, &Oa, FALSE, TRUE ))) {
        CallbackHandle = ExRegisterCallback( CallbackObject,
                                             (PCALLBACK_FUNCTION)CallbackFunction,
                                             CallbackContext );

        //
        //  Remove the residual reference on failure or the registration
        //  reference.
        //
        ObDereferenceObject( CallbackObject );
    }

    return CallbackHandle;
}

_IRQL_requires_max_( APC_LEVEL )
_IRQL_requires_same_
VOID
DsDeregisterPowerChangeCallback(
    _In_ PVOID CallbackHandle
    )
{
    if (CallbackHandle) {
        ExUnregisterCallback( CallbackHandle );
    }
}