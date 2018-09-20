#include "wdk7.h"
#include "hvm.h"
#include "smp.h"

VOID
HvmSetHvmContext(
    _Inout_ PHVM Hvm,
    _In_ PVOID Context
    )
{
    NT_ASSERT( Hvm );
    InterlockedExchangePointer( &Hvm->HvmContext, Context );
}

PVOID
HvmGetHvmContext(
    _In_ PHVM Hvm
    )
{
    NT_ASSERT( Hvm );
    return Hvm->HvmContext;
}

PVOID
HvmGetHvmContextByVcpu(
    _In_ PHVM_VCPU Vcpu
    )
{
    NT_ASSERT( Vcpu );
    return Vcpu->Hvm->HvmContext;
}

VOID
HvmSetVcpuContext(
    _Inout_ PHVM Hvm,
    _In_ UINT32 VcpuId,
    _In_ PVOID Context
    )
{
    NT_ASSERT( Hvm );
    NT_ASSERT( VcpuId < SmpActiveProcessorCount() );

    InterlockedExchangePointer( &Hvm->VcpuArray[VcpuId].Context, Context );
}

PVOID
HvmGetVcpuContext(
    _In_ PHVM Hvm,
    _In_ UINT32 VcpuId
    )
{
    NT_ASSERT( Hvm );
    NT_ASSERT( VcpuId < SmpActiveProcessorCount() );

    return Hvm->VcpuArray[VcpuId].Context;
}

PVOID
HvmGetVcpuContextByVcpu(
    _In_ PHVM_VCPU Vcpu
    )
{
    NT_ASSERT( Vcpu );
    return Vcpu->Context;
}