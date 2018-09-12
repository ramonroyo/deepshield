#ifndef __HVM_H__
#define __HVM_H__

#include "x86.h"
#include "sync.h"

typedef struct DECLSPEC_ALIGN(16) _REGISTERS
{
    UINT_PTR rax;
    UINT_PTR rcx;
    UINT_PTR rdx;
    UINT_PTR rbx;
    UINT_PTR rsp;
    UINT_PTR rbp;
    UINT_PTR rsi;
    UINT_PTR rdi;

#ifdef _WIN64
    UINT_PTR r8;
    UINT_PTR r9;
    UINT_PTR r10;
    UINT_PTR r11;
    UINT_PTR r12;
    UINT_PTR r13;
    UINT_PTR r14;
    UINT_PTR r15;
#endif

#if defined(_WIN64)
    M128A XmmRegisters[16];
#else
    M128A XmmRegisters[8];
#endif

    UINT_PTR rip;
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

typedef struct _HVM_VCPU HVM_VCPU, *PHVM_VCPU;

typedef 
VOID(*PHVM_EXIT_HANDLER)(
    _In_ UINT32     exitReason,
    _In_ PHVM_VCPU  Vcpu,
    _In_ PREGISTERS regs
    );

typedef VOID(*PHVM_SETUP_VMCS)(
    _In_ PHVM_VCPU Vcpu
    );

typedef struct _HVM HVM, *PHVM;

typedef struct _HVM_VCPU
{
    //
    //  This field must be the first.
    //
    REGISTERS GuestRegisters;
    UINT32 Index;
    PHVM Hvm;
    HOST_SAVED_STATE SavedState;
    PVOID VmxOnRegionHva;
    PHYSICAL_ADDRESS VmxOnRegionHpa;
    PVOID VmcsRegionHva;
    PHYSICAL_ADDRESS VmcsRegionHpa;
    PVOID Stack;
    UINT_PTR Rsp;
    PHVM_EXIT_HANDLER ExitHandler;
    PHVM_SETUP_VMCS SetupVmcs;
    PVOID LocalContext;
    ATOMIC Launched;
    HVM_LOGGED_EVENTS LoggedEvents;
} HVM_VCPU, *PHVM_VCPU;

typedef struct _HVM
{
    PHVM_VCPU VcpuArray;
    PVOID globalContext;
    ATOMIC launched;
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
HvmInitialize(
    _In_ UINT32 StackPages,
    _In_ PHVM_EXIT_HANDLER ExitHandlerCb,
    _In_ PHVM_SETUP_VMCS SetupVmcsCb
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
    _In_ UINT32 VcpuId,
    _In_ PVOID  context
);

PVOID
HvmLocalContextGet(
    _In_ UINT32 VcpuId
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
HvmFinalize(
    VOID
);

#define ROOT_MODE_API

PHVM_VCPU ROOT_MODE_API
HvmGetCurrentVcpu(
    VOID
);

PVOID ROOT_MODE_API
HvmGetVcpuLocalContext(
    _In_ PHVM_VCPU Vcpu
);

PVOID ROOT_MODE_API
HvmGetVcpuGlobalContext(
    _In_ PHVM_VCPU Vcpu
);

UINT32 ROOT_MODE_API
HvmGetVcpuId(
    _In_ PHVM_VCPU Vcpu
);

PHVM ROOT_MODE_API
HvmGetVcpuHvm(
    _In_ PHVM_VCPU Vcpu
);

PHOST_SAVED_STATE ROOT_MODE_API
HvmGetVcpuSavedState(
    _In_ PHVM_VCPU Vcpu
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
HvmVcpuCommonExitsHandler(
    _In_ UINT32     exitReason,
    _In_ PHVM_VCPU  Vcpu,
    _In_ PREGISTERS regs
);

#endif
