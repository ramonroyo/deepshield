#ifndef __DISASSM_H__
#define __DISASSM_H__
#include "dsdef.h"
#include <capstone.h>

#define MAX_INST_SIZE 15

#define CS_POOL_TAG 'aMsC'

//
// Type redefinitions
//

typedef cs_insn CS_INSN, *PCS_INSN;
typedef cs_x86 CS_X86, *PCS_X86;
typedef cs_x86_op CS_X86_OP, *PCS_X86_OP;
typedef csh CS_HANDLE, *PCS_HANDLE;
typedef cs_err CS_ERR;
typedef cs_opt_type CS_OPT_TYPE;

VOID
CsDestroy(
    _In_ PCS_HANDLE CsHandle,
    _In_ PCS_INSN   Instruction
    );

PCS_INSN CsInitialize(
    _Inout_ PCS_HANDLE CsHandle
    );

#endif


