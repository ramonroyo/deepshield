#ifndef __EXITS_H__
#define __EXITS_H__

#include "hvm.h"

VOID 
DeepShieldExitHandler(
    _In_ UINT32     exitReason,
    _In_ PHVM_CORE  core,
    _In_ PREGISTERS regs
);

#endif
