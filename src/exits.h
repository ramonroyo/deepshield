#ifndef __EXITS_H__
#define __EXITS_H__

#include "hvm.h"

VOID 
DsHvmExitHandler(
    _In_ UINT32 exitReason,
    _In_ PHVM_VCPU  Vcpu,
    _In_ PGP_REGISTERS Registers
);

#endif
