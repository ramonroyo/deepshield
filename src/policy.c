/*++

Copyright (c) ByteHeed. All Rights Reserved.

Module Name:

    policy.c

Abstract:

    This file implements the utility functions needed to set the initial
    shield policy.

Environment:

--*/

#include "dsdef.h"

#if defined(WPP_EVENT_TRACING)
#include "policy.tmh"
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, DsOpenPolicyKey)
#pragma alloc_text(PAGE, DsClosePolicyKey)
#pragma alloc_text(PAGE, DsQueryLoadModePolicy)
#pragma alloc_text(PAGE, DsSetLoadModePolicy)
#endif

NTSTATUS
DsOpenPolicyKey(
    _Out_ PHANDLE PolicyKey
    )
{
    HANDLE RootKey = NULL;
    OBJECT_ATTRIBUTES KeyAttributes;
    UNICODE_STRING KeyName;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    PAGED_CODE();

    NT_ASSERT( PolicyKey );
    *PolicyKey = NULL;

    if (NULL == gDriverKeyName.Buffer) {
        goto RoutineExit;
    }

    InitializeObjectAttributes( &KeyAttributes,
                                &gDriverKeyName,
                                OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                NULL,
                                NULL );

    Status = ZwOpenKey( &RootKey, KEY_ALL_ACCESS, &KeyAttributes );

    if (NT_SUCCESS( Status )) {

        RtlInitUnicodeString( &KeyName, DSH_POLICY_KEY_NAME);

        RtlZeroMemory( &KeyAttributes, sizeof( OBJECT_ATTRIBUTES ));
        InitializeObjectAttributes( &KeyAttributes,
                                    &KeyName,
                                    OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                    RootKey,
                                    NULL);

        Status = ZwCreateKey( PolicyKey,
                              KEY_ALL_ACCESS,
                              &KeyAttributes,
                              0,
                              NULL,
                              REG_OPTION_NON_VOLATILE,
                              NULL );

        if (!NT_SUCCESS( Status )) {
            *PolicyKey = NULL;
        }
    }

RoutineExit:

    if (RootKey) {
        ZwClose( RootKey );
    }

    return Status;
}

VOID
DsClosePolicyKey(
    _In_ HANDLE PolicyKey
    )
{
    PAGED_CODE();
    ZwClose( PolicyKey );
}

NTSTATUS
DsSetLoadModePolicy(
    _In_ ULONG LoadMode
    )
{
    HANDLE PolicyKey;
    UNICODE_STRING ValueName;
    NTSTATUS Status;

    PAGED_CODE();

    Status = DsOpenPolicyKey( &PolicyKey );

    if (NT_SUCCESS( Status )) {
        RtlInitUnicodeString( &ValueName, DSH_RUN_MODE_POLICY );

        Status = ZwSetValueKey( PolicyKey,
                                &ValueName,
                                0,
                                REG_DWORD,
                                &LoadMode,
                                sizeof( ULONG ));

        DsClosePolicyKey( PolicyKey );
    }

    return Status;
}

NTSTATUS
DsQueryLoadModePolicy(
    _Out_ PULONG LoadMode
    )
{
    NTSTATUS Status;
    HANDLE PolicyKey;
    UCHAR Buffer[sizeof( KEY_VALUE_PARTIAL_INFORMATION ) + sizeof( LONG )];
    PKEY_VALUE_PARTIAL_INFORMATION ValuePartialInfo;
    UNICODE_STRING ValueName;
    ULONG ResultLength;

    PAGED_CODE();

    NT_ASSERT( LoadMode );
    *LoadMode = 0;

    Status = DsOpenPolicyKey( &PolicyKey );

    if (NT_SUCCESS( Status )) {
        RtlInitUnicodeString( &ValueName, DSH_RUN_MODE_POLICY );

        Status = ZwQueryValueKey( PolicyKey,
                                  &ValueName,
                                  KeyValuePartialInformation,
                                  Buffer,
                                  sizeof( Buffer ),
                                  &ResultLength );

        if (NT_SUCCESS( Status )) {
            ValuePartialInfo = (PKEY_VALUE_PARTIAL_INFORMATION) Buffer;

            if (ValuePartialInfo->Type == REG_DWORD) {
                *LoadMode = *((PLONG)&(ValuePartialInfo->Data));
            }
        }

        DsClosePolicyKey( PolicyKey );
    }
    
    return Status;
}