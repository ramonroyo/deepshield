#include "wdk7.h"
#include "channel.h"
#include "shield.h"
#include "dshvm.h"
#include "mmu.h"
#include "mem.h"
#include "hvm.h"
#include "vmx.h"
#include "smp.h"

extern UINTN gSystemPageDirectoryTable;

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
    return DsLoadHvm();
}

NTSTATUS
DsStopShield(
    VOID
    )
{
    return DsUnloadHvm();
}

BOOLEAN
DsIsShieldRunning(
    VOID
    )
{
    return DsIsHvmRunning();
}