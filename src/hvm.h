#ifndef __HVM_H__
#define __HVM_H__

#include "x86.h"
#include "sync.h"

typedef struct _REGISTERS
{
#ifdef _WIN64
    UINT_PTR r15;
    UINT_PTR r14;
    UINT_PTR r13;
    UINT_PTR r12;
    UINT_PTR r11;
    UINT_PTR r10;
    UINT_PTR r9;
    UINT_PTR r8;
#endif
    UINT_PTR rdi;
    UINT_PTR rsi;
    UINT_PTR rbp;
    UINT_PTR rsp;
    UINT_PTR rbx;
    UINT_PTR rdx;
    UINT_PTR rcx;
    UINT_PTR rax;

    UINT_PTR       rip;
    FLAGS_REGISTER rflags;
} REGISTERS, *PREGISTERS;

typedef struct _DESCRIPTOR_TABLE_REGISTER
{
    UINT_PTR base;
    UINT16   limit;
} DESCRIPTOR_TABLE_REGISTER, *PDESCRIPTOR_TABLE_REGISTER;

typedef struct _HOST_SAVED_STATE
{
    CR0_REGISTER              cr0;
    CR4_REGISTER              cr4;
    SEGMENT_SELECTOR          cs;
    SEGMENT_SELECTOR          ss;
    SEGMENT_SELECTOR          ds;
    SEGMENT_SELECTOR          es;
    SEGMENT_SELECTOR          fs;
    UINT_PTR                  fsBase;
    SEGMENT_SELECTOR          gs;
    UINT_PTR                  gsBase;
    SEGMENT_SELECTOR          tr;
    UINT_PTR                  trBase;
    DESCRIPTOR_TABLE_REGISTER gdt;
    DESCRIPTOR_TABLE_REGISTER idt;
    UINT32                    sysenterCs;
    UINT_PTR                  sysenterEsp;
    UINT_PTR                  sysenterEip;
    UINT64                    perfGlobalCtrl;
    UINT64                    pat;
    UINT64                    efer;
} HOST_SAVED_STATE, *PHOST_SAVED_STATE;

typedef struct _EXIT_INFO
{
    UINT32   exitReason;
    UINT_PTR exitQualification;
    UINT_PTR guestLinearAddress;
    UINT64   guestPhyscalAddress;
    UINT32   exitInterruptionInformation;
    UINT32   exitInterruptionErrorCode;
    UINT32   idtVectoringInformation;
    UINT32   idtVectoringErrorCode;
    UINT32   exitInstructionLength;
    UINT32   exitInstructionInformation;
    UINT_PTR ioRcx;
    UINT_PTR ioRsi;
    UINT_PTR ioRdi;
    UINT_PTR ioRip;
    UINT_PTR vmInstructionError;
} EXIT_INFO, *PEXIT_INFO;

typedef struct _HVM_EVENT
{
    EXIT_INFO info;
    REGISTERS regs;
} HVM_EVENT, *PHVM_EVENT;

#define MAX_NUMBER_OF_LOGGED_EXIT_EVENTS 4

typedef struct _HVM_LOGGED_EVENTS
{
    UINT_PTR  numberOfEvents;
    HVM_EVENT queue[MAX_NUMBER_OF_LOGGED_EXIT_EVENTS];
} HVM_LOGGED_EVENTS, *PHVM_LOGGED_EVENTS;

typedef struct _HVM_CORE HVM_CORE, *PHVM_CORE;

typedef VOID(*HVM_EXIT_HANDLER)(
    _In_ UINT32     exitReason,
    _In_ PHVM_CORE  core,
    _In_ PREGISTERS regs
    );

typedef VOID(*HVM_CONFIGURE)(
    _In_ PHVM_CORE core
    );

typedef struct _HVM HVM, *PHVM;

typedef struct _HVM_CORE
{
    UINT32   index;
    PHVM     hvm;

    HOST_SAVED_STATE savedState;
    PVOID            vmxOn;
    PVOID            vmcs;
    PVOID            stack;
    UINT_PTR         rsp;
    HVM_EXIT_HANDLER handler;
    HVM_CONFIGURE    configure;
    PVOID            localContext;

    ATOMIC            launched;
    HVM_LOGGED_EVENTS loggedEvents;
} HVM_CORE, *PHVM_CORE;

typedef struct _HVM
{
    PHVM_CORE cores;
    PVOID     globalContext;

    ATOMIC    launched;
} HVM, *PHVM;


BOOLEAN
HvmInitialized(
    VOID
);

BOOLEAN
HvmLaunched(
    VOID
);


NTSTATUS
HvmInit(
    _In_  UINT32           stackPages,
    _In_  HVM_EXIT_HANDLER handler,
    _In_  HVM_CONFIGURE    configure
);

VOID
HvmGlobalContextSet(
    _In_ PVOID context
);

PVOID
HvmGlobalContextGet(
    VOID
);

VOID
HvmLocalContextSet(
    _In_ UINT32 core,
    _In_ PVOID  context
);

PVOID
HvmLocalContextGet(
    _In_ UINT32 core
);

NTSTATUS
HvmStart(
    VOID
);

NTSTATUS
HvmStop(
    VOID
);

NTSTATUS
HvmDone(
    VOID
);








#define ROOT_MODE_API

PHVM_CORE ROOT_MODE_API
HvmCoreCurrent(
    VOID
);

PREGISTERS ROOT_MODE_API
HvmCoreRegisters(
    _In_ PHVM_CORE core
);

PVOID ROOT_MODE_API
HvmCoreLocalContext(
    _In_ PHVM_CORE core
);

PVOID ROOT_MODE_API
HvmCoreGlobalContext(
    _In_ PHVM_CORE core
);

UINT32 ROOT_MODE_API
HvmCoreIndex(
    _In_ PHVM_CORE core
);

PHVM ROOT_MODE_API
HvmCoreHvm(
    _In_ PHVM_CORE core
);

PHOST_SAVED_STATE ROOT_MODE_API
HvmCoreSavedState(
    _In_ PHVM_CORE core
);


/*
* Will handle in a passtrough mode:
*   EXIT_REASON_INVD
*   EXIT_REASON_XSETBV
*   EXIT_REASON_VMXON
*   EXIT_REASON_VMXOFF
*   EXIT_REASON_VMCLEAR
*   EXIT_REASON_VMRESUME
*   EXIT_REASON_VMLAUNCH
*   EXIT_REASON_VMPTRLD
*   EXIT_REASON_VMPTRST
*   EXIT_REASON_VMREAD
*   EXIT_REASON_VMWRITE
*   EXIT_REASON_INVEPT
*   EXIT_REASON_INVVPID
*   EXIT_REASON_CR_ACCESS
*   EXIT_REASON_MSR_READ
*   EXIT_REASON_MSR_WRITE
*   EXIT_REASON_CPUID
*/
BOOLEAN ROOT_MODE_API
HvmCoreHandleCommonExits(
    _In_ UINT32     exitReason,
    _In_ PHVM_CORE  core,
    _In_ PREGISTERS regs
);

#endif
