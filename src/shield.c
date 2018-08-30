#include "wdk7.h"
#include "channel.h"
#include "shield.h"
#include "mmu.h"
#include "mem.h"
#include "hvm.h"
#include "vmx.h"
#include "hvds.h"
#include "smp.h"

extern UINT_PTR gSystemPageDirectoryTable;

NTSTATUS
DsInitializeShield(
    VOID
    )
{
    gSystemPageDirectoryTable = __readcr3();
    return MmuInitialize();
}

VOID
DsFinalizeShield(
    VOID
    )
{
    MmuFinalize();
}

NTSTATUS
DsStartShield(
    VOID
    )
{
    return DsLoadHvds();
}

NTSTATUS
DsStopShield(
    VOID
    )
{
    return DsUnloadHvds();
}

BOOLEAN
DsIsShieldRunning(
    VOID
    )
{
    return DsIsHvdsRunning();
}