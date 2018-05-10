#include "wdk7.h"
#include "shield.h"
#include "mmu.h"
#include "mem.h"
#include "hvm.h"
#include "vmx.h"
#include "hvds.h"
#include "smp.h"

NTSTATUS
DsInitializeShield(
    VOID
    )
{
    NTSTATUS Status;

    Status = DsIsHvdsSupported();
    if (!NT_SUCCESS( Status )) {
        return Status;
    }

    Status = MmuInit();
    if (!NT_SUCCESS( Status )) {
        return Status;
    }

    Status = DsInitializeHvds();
    if (!NT_SUCCESS( Status )) {
        return Status;
    }

    return Status;
}

VOID
DsFinalizeShield(
    VOID
    )
{
    DsFinalizeHvds();
    MmuDone();
}

NTSTATUS
DsStartShield(
    VOID
    )
{
    NTSTATUS Status;

    Status = DsStartHvds();

    if (!NT_SUCCESS( Status )) {
        DsStopHvds();
        return Status;
    }

    return Status;
}

NTSTATUS
DsStopShield(
    VOID
    )
{
    return DsStopHvds();
}

BOOLEAN
DsIsShieldRunning(
    VOID
    )
{
    return DsIsHvdsRunning();
}
