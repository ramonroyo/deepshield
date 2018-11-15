/*++

Copyright (C) ByteHeed. All Rights Reserved

Module Name:

    policy.h

--*/

#ifndef _POLICY_H_
#define _POLICY_H_

#include <ntifs.h>
#include "wdk7.h"

NTSTATUS
DsOpenPolicyKey(
    _Out_ PHANDLE PolicyKey
    );

VOID
DsClosePolicyKey(
    _In_ HANDLE PolicyKey
    );

NTSTATUS
DsSetLoadModePolicy(
    _In_ ULONG LoadMode
    );

NTSTATUS
DsQueryLoadModePolicy(
    _Out_ PULONG LoadMode
    );

#endif