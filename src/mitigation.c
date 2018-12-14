/*++

Copyright (c) ByteHeed. All Rights Reserved.

Module Name:

    mitigation.c

Abstract:

    This file implements support routines for multiple side channel mitigation
    techniques introduced by Intel's microcode updates.

Environment:

--*/

#include "dsdef.h"

#if defined(WPP_EVENT_TRACING)
#include "mitigation.tmh"
#endif

BOOLEAN
DsCheckCpuSpeculationControlCapable(
    VOID
    )
/*++

Routine Description:

    This routine checks whether the processor provides a method for critical
    software to protect their indirect branch predictions, by invalidating them
    at appropriate times with IBRS and IBPB.

    When software sets IA32_SPEC_CTRL.IBRS to 1 after a transition to a more
    privileged predictor mode, predicted targets of indirect branches executed
    in that predictor mode cannot be controlled by software that was executed
    in a less privileged predictor mode.
    
    Additionally, enabling IBRS prevents software operating on one logical
    processor from controlling the predicted targets of indirect branches
    executed on another logical processor.
    
Arguments:
    None
    
Return value:
    BOOLEAN

--*/
{
    CPU_INFO CpuInfo = { 0 };

    //
    //  Note that a processor may enumerate support for the IA32_SPEC_CTRL
    //  MSR but not for STIBP.
    //
    __cpuid( &CpuInfo, CPUID_EXTENDED_FEATURE_FLAGS );

    if (CPUID_VALUE_EDX( CpuInfo ) & CPUID_LEAF_7H_0H_EDX_IBRS) {
        //
        //  IBRS always implies IBPB.
        //
        return TRUE;
    }

    return FALSE;
}

VOID
DsSetIndirectBranchPredictorBarrier(
    VOID
    )
/*++

Routine Description:

    This routine establishes a barrier by issuing a IBPD command. This command
    prevents software from controlling the predicted target of an indirect
    branch of unrelated software (e.g., a different virtual machine) executed
    at the same predictor mode. Hence it is used when changing the identity of
    software operating at a particular predictor mode (e.g., when changing
    between virtual machines).

Arguments:
    None
    
Return value:
    None

--*/
{
    //
    //  The barrier can be set also as part of a VMX transition that loads the
    //  MSR from an MSR load area
    //
    __writemsr( IA32_PRED_CMD, PRED_CMD_FEATURE_SET_IBPB );
}

BOOLEAN
DsCheckCpuFlushCommandCapable(
    VOID
    )
/*++

Routine Description:

    This routine determines whether the flush command capability is present.
    The L1D_FLUSH command allows for finer granularity invalidation of caching
    structures than existing mechanisms like WBINVD. It will writeback and
    invalidate the L1 data cache, including all cachelines brought in by
    preceding instructions, without invalidating all caches( for example, L2
    or LLC ). Some processors may also invalidate the first level instruction
    cache on a L1D_FLUSH command. The L1 data and instruction caches may be
    shared across the logical processors of a core. This command can be used
    by a VMM to mitigate the L1TF exploit.

Arguments:
    None
    
Return value:
    BOOLEAN

--*/
{
    CPU_INFO CpuInfo = { 0 };

    __cpuid( &CpuInfo, CPUID_EXTENDED_FEATURE_FLAGS );

    if (CPUID_VALUE_EDX( CpuInfo ) & CPUID_LEAF_7H_0H_EDX_L1D_FLUSH ) {
        return TRUE;
    }

    return FALSE;
}

BOOLEAN
DsQueryCpuArchCapabilities(
    PMSR_ARCH_CAPS ArchCapabilities
    )
/*++

Routine Description:

    This routine queries the IA32_ARCH_CAPABILITIES MSR after checking that
    the processor explicitly enumerates support for it.

Arguments:
    None
    
Return value:
    BOOLEAN

--*/
{
    CPU_INFO CpuInfo = { 0 };

    __cpuid( &CpuInfo, CPUID_EXTENDED_FEATURE_FLAGS );

    if (CPUID_VALUE_EDX( CpuInfo ) & CPUID_LEAF_7H_0H_EDX_ARCH_CAPS ) {

        ArchCapabilities->AsUint64 = __readmsr( IA32_ARCH_CAPABILITIES );
        return TRUE;
    }

    return FALSE;
}

BOOLEAN
DsCheckCpuSsbdCapable(
    VOID
    )
/*++

Routine Description:

    This routine returns the Speculative Store Bypass Disable capability. When
    SSBD is set, loads will not execute speculatively until the addresses of
    all older stores are known. This ensures that a load does not speculatively
    consume stale data values due to bypassing an older store on the same
    logical processor.

Arguments:
    None
    
Return value:
    BOOLEAN

--*/
{
    CPU_INFO CpuInfo = { 0 };

    __cpuid( &CpuInfo, CPUID_EXTENDED_FEATURE_FLAGS );

    if (CPUID_VALUE_EDX( CpuInfo ) & CPUID_LEAF_7H_0H_EDX_SSBD) {
        return TRUE;
    }

    return FALSE;
}

