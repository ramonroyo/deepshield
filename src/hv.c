#include "hv.h"
#include "mmu.h"
#include "mem.h"
#include "hvm.h"
#include "vmx.h"
#include "deepshield.h"
#include "smp.h"

NTSTATUS
HvInit(
    VOID
)
{
    NTSTATUS status;

    status = DeepShieldIsSupported();
    if (!NT_SUCCESS(status))
        return status;

    status = MemInit((SmpNumberOfCores() + 1) * 8 * PAGE_SIZE);
    if (!NT_SUCCESS(status))
        return status;

    status = MmuInit();
    if (!NT_SUCCESS(status))
        return status;

    status = DeepShieldInit();
    if (!NT_SUCCESS(status))
        return status;

    return status;
}


VOID
HvDone(
    VOID
)
{
	DeepShieldDone();
    MmuDone();
    MemDone();
}


NTSTATUS
HvStart(
    VOID
)
{
    NTSTATUS status;

    status = DeepShieldStart();
    if (!NT_SUCCESS(status))
    {
		DeepShieldStop();
        return status;
    }


    return status;
}


VOID
HvStop(
    VOID
)
{
	DeepShieldStop();
}
