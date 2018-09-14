#ifndef __HVM_H__
#define __HVM_H__

#include "x86.h"
#include "sync.h"

typedef struct DECLSPEC_ALIGN(16) _GP_REGISTERS
{
    UINTN Rax;
    UINTN Rcx;
    UINTN Rdx;
    UINTN Rbx;
    UINTN Rsp;
    UINTN Rbp;
    UINTN Rsi;
    UINTN Rdi;

#ifdef _WIN64
    UINTN R8;
    UINTN R9;
    UINTN R10;
    UINTN R11;
    UINTN R12;
    UINTN R13;
    UINTN R14;
    UINTN R15;
#endif

#if defined(_WIN64)
    M128A XmmRegisters[16];
#else
    M128A XmmRegisters[8];
#endif

    UINTN Rip;
    FLAGS_REGISTER Rflags;

} GP_REGISTERS, *PGP_REGISTERS;

typedef struct _HOST_SAVED_STATE
{
    CR0_REGISTER              Cr0;
    CR4_REGISTER              Cr4;
    IA32_SEGMENT_SELECTOR          Cs;
    IA32_SEGMENT_SELECTOR          Ss;
    IA32_SEGMENT_SELECTOR          Ds;
    IA32_SEGMENT_SELECTOR          Es;
    IA32_SEGMENT_SELECTOR          Fs;
    UINTN                  FsBase;
    IA32_SEGMENT_SELECTOR          Gs;
    UINTN                  GsBase;
    IA32_SEGMENT_SELECTOR          Tr;
    UINTN                  TrBase;
    IA32_DESCRIPTOR           Gdt;
    IA32_DESCRIPTOR           Idt;
    UINT32                    SysenterCs;
    UINTN                  SysenterEsp;
    UINTN                  SysenterEip;
    UINT64                    PerfGlobalCtrl;
    UINT64                    Pat;
    UINT64                    Efer;
} HOST_SAVED_STATE, *PHOST_SAVED_STATE;

typedef struct _EXIT_INFO
{
    UINT32   exitReason;
    UINTN exitQualification;
    UINTN guestLinearAddress;
    UINT64   guestPhyscalAddress;
    UINT32   exitInterruptionInformation;
    UINT32   exitInterruptionErrorCode;
    UINT32   idtVectoringInformation;
    UINT32   idtVectoringErrorCode;
    UINT32   exitInstructionLength;
    UINT32   exitInstructionInformation;
    UINTN ioRcx;
    UINTN ioRsi;
    UINTN ioRdi;
    UINTN ioRip;
    UINTN vmInstructionError;
} EXIT_INFO, *PEXIT_INFO;

typedef struct _HVM_EVENT
{
    EXIT_INFO info;
    GP_REGISTERS Registers;
} HVM_EVENT, *PHVM_EVENT;

#define MAX_NUMBER_OF_LOGGED_EXIT_EVENTS 4

typedef struct _HVM_LOGGED_EVENTS
{
    UINTN  numberOfEvents;
    HVM_EVENT queue[MAX_NUMBER_OF_LOGGED_EXIT_EVENTS];
} HVM_LOGGED_EVENTS, *PHVM_LOGGED_EVENTS;

typedef struct _HVM_VCPU HVM_VCPU, *PHVM_VCPU;

typedef 
VOID(*PHVM_EXIT_HANDLER)(
    _In_ UINT32 ExitReason,
    _In_ PHVM_VCPU  Vcpu,
    _In_ PGP_REGISTERS Registers
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
    GP_REGISTERS GuestRegisters;
    UINT32 Index;
    PHVM Hvm;
    HOST_SAVED_STATE HostState;
    PVOID VmxOnHva;
    PHYSICAL_ADDRESS VmxOnHpa;
    PVOID VmcsHva;
    PHYSICAL_ADDRESS VmcsHpa;
    PVOID Stack;
    UINTN Rsp;
    PHVM_EXIT_HANDLER ExitHandler;
    PHVM_SETUP_VMCS SetupVmcs;
    PVOID LocalContext;
    ATOMIC Launched;
    HVM_LOGGED_EVENTS LoggedEvents;
} HVM_VCPU, *PHVM_VCPU;

typedef struct _HVM
{
    PHVM_VCPU VcpuArray;
    PVOID HvmContext;
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
HvmGetVcpuContext(
    _In_ PHVM_VCPU Vcpu
);

PVOID ROOT_MODE_API
HvmGetHvmContext(
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
HvmGetVcpuHostState(
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
    _In_ PGP_REGISTERS Registers
);

#endif
