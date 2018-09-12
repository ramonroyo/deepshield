/**
*  @file    VMX.h
*  @brief   Virtual Machine for Intel interface.
*
*  Functionality for virtualization technology on Intel based systems.
*/
#ifndef __VMX_H__
#define __VMX_H__

#include <ntifs.h>

//
//  VMX capabilities are declared in bit 5 of ECX retured from CPUID
//
#define IA32_CPUID_ECX_VMX                  0x20

//
// VMX NTSTATUS Errors
//
#define VMX_STATUS_MASK  0xEB1E0000
#define VMX_STATUS(X)    ((NTSTATUS)(VMX_STATUS_MASK | ((X) & 0x0000FFFF)))

//
//  VMX Error Codes
//
#define NO_INSTRUCTION_ERROR                        0                  /* VMxxxxx */
#define VMCALL_IN_ROOT_ERROR                        1                  /* VMCALL */
#define VMCLEAR_INVALID_PHYSICAL_ADDRESS_ERROR      2                  /* VMCLEAR */
#define VMCLEAR_WITH_CURRENT_CONTROLLING_PTR_ERROR  3                  /* VMCLEAR */
#define VMLAUNCH_WITH_NON_CLEAR_VMCS_ERROR          4                  /* VMLAUNCH */
#define VMRESUME_WITH_NON_LAUNCHED_VMCS_ERROR       5                  /* VMRESUME */
#define VMRESUME_WITH_NON_CHILD_VMCS_ERROR          6                  /* VMRESUME */
#define VMENTER_BAD_CONTROL_FIELD_ERROR             7                  /* VMENTER */
#define VMENTER_BAD_MONITOR_STATE_ERROR             8                  /* VMENTER */
#define VMPTRLD_INVALID_PHYSICAL_ADDRESS_ERROR      9                  /* VMPTRLD */
#define VMPTRLD_WITH_CURRENT_CONTROLLING_PTR_ERROR  10                 /* VMPTRLD */
#define VMPTRLD_WITH_BAD_REVISION_ID_ERROR          11                 /* VMPTRLD */
#define VMREAD_OR_VMWRITE_OF_UNSUPPORTED_COMPONENT_ERROR 12            /* VMREAD */
#define VMWRITE_OF_READ_ONLY_COMPONENT_ERROR        13                 /* VMWRITE */
#define VMWRITE_INVALID_FIELD_VALUE_ERROR           14                 /* VMWRITE */
#define VMXON_IN_VMX_ROOT_OPERATION_ERROR           15                 /* VMXON */
#define VMENTRY_WITH_BAD_OSV_CONTROLLING_VMCS_ERROR 16                 /* VMENTER */
#define VMENTRY_WITH_NON_LAUNCHED_OSV_CONTROLLING_VMCS_ERROR 17        /* VMENTER */
#define VMENTRY_WITH_NON_ROOT_OSV_CONTROLLING_VMCS_ERROR 18            /* VMENTER */
#define VMCALL_WITH_NON_CLEAR_VMCS_ERROR            19                 /* VMCALL */
#define VMCALL_WITH_BAD_VMEXIT_FIELDS_ERROR         20                 /* VMCALL */
#define VMCALL_WITH_INVALID_MSEG_MSR_ERROR          21                 /* VMCALL */
#define VMCALL_WITH_INVALID_MSEG_REVISION_ERROR     22                 /* VMCALL */
#define VMXOFF_WITH_CONFIGURED_SMM_MONITOR_ERROR    23                 /* VMXOFF */
#define VMCALL_WITH_BAD_SMM_MONITOR_FEATURES_ERROR  24                 /* VMCALL */
#define RETURN_FROM_SMM_WITH_BAD_VM_EXECUTION_CONTROLS_ERROR 25        /* Return from SMM */
#define VMENTRY_WITH_EVENTS_BLOCKED_BY_MOV_SS_ERROR 26                 /* VMENTER */
#define BAD_ERROR_CODE                              27                 /* Bad error code */
#define INVALIDATION_WITH_INVALID_OPERAND           28                 /* INVEPT, INVVPID */

//
//  VMCS VM_INSTRUCTION_ERROR field.
//
#define STATUS_VM_ENTRY_WITH_INVALID_CONTROL_FIELDS                             VMX_STATUS(VMENTER_BAD_CONTROL_FIELD_ERROR)
#define STATUS_VM_ENTRY_WITH_INVALID_HOST_STATE_FIELDS                          VMX_STATUS(VMENTER_BAD_MONITOR_STATE_ERROR)
#define STATUS_VM_ENTRY_WITH_INVALID_EXECUTIVE_VMCS_POINTER                     VMX_STATUS(VMENTRY_WITH_BAD_OSV_CONTROLLING_VMCS_ERROR)
#define STATUS_VM_ENTRY_WITH_NON_LAUNCHED_EXECUTIVE_VMCS                        VMX_STATUS(VMENTRY_WITH_NON_LAUNCHED_OSV_CONTROLLING_VMCS_ERROR)
#define STATUS_VM_ENTRY_WITH_EXECUTIVE_VMCS_POINTER_NOT_VMXON                   VMX_STATUS(VMENTRY_WITH_NON_ROOT_OSV_CONTROLLING_VMCS_ERROR)
#define STATUS_VM_ENTRY_WITH_INVALID_VM_EXEC_CONTROL_FIELDS_IN_EXECUTIVE_VMCS   VMX_STATUS(RETURN_FROM_SMM_WITH_BAD_VM_EXECUTION_CONTROLS_ERROR)
#define STATUS_VM_ENTRY_WITH_EVENTS_BLOCKED_BY_MOV_SS                           VMX_STATUS(VMENTRY_WITH_EVENTS_BLOCKED_BY_MOV_SS_ERROR)
#define STATUS_VMCALL_EXECUTED_IN_VMX_ROOT_OPERATION                            VMX_STATUS(VMCALL_IN_ROOT_ERROR)
#define STATUS_VMCALL_WITH_NON_CLEAR_VMCS                                       VMX_STATUS(VMCALL_WITH_NON_CLEAR_VMCS_ERROR)
#define STATUS_VMCALL_WITH_INVALID_VM_EXIT_CONTROL_FIELDS                       VMX_STATUS(VMCALL_WITH_BAD_VMEXIT_FIELDS_ERROR)
#define STATUS_VMCALL_WITH_INCORRECT_MSEG_REVISION_ID                           VMX_STATUS(VMCALL_WITH_INVALID_MSEG_REVISION_ERROR)
#define STATUS_VMCALL_WITH_INVALID_SMM_MONITOR_FEATURES                         VMX_STATUS(VMCALL_WITH_BAD_SMM_MONITOR_FEATURES_ERROR)
#define STATUS_VMPTRLD_WITH_INVALID_PHYSICAL_ADDRESS                            VMX_STATUS(VMPTRLD_INVALID_PHYSICAL_ADDRESS_ERROR)
#define STATUS_VMPTRLD_WITH_VMXON_POINTER                                       VMX_STATUS(VMPTRLD_WITH_CURRENT_CONTROLLING_PTR_ERROR)
#define STATUS_VMPTRLD_WITH_INCORRECT_VMCS_REVISION_IDENTIFIER                  VMX_STATUS(VMPTRLD_WITH_BAD_REVISION_ID_ERROR)
#define STATUS_VMCLEAR_WITH_INVALID_PHYSICAL_ADDRESS                            VMX_STATUS(VMCLEAR_INVALID_PHYSICAL_ADDRESS_ERROR)
#define STATUS_VMCLEAR_WITH_VMXON_POINTER                                       VMX_STATUS(VMCLEAR_WITH_CURRENT_CONTROLLING_PTR_ERROR)
#define STATUS_VMLAUNCH_WITH_NON_CLEAR_VMCS                                     VMX_STATUS(VMLAUNCH_WITH_NON_CLEAR_VMCS_ERROR)
#define STATUS_VMRESUME_WITH_NON_LAUNCHED_VMCS                                  VMX_STATUS(VMRESUME_WITH_NON_LAUNCHED_VMCS_ERROR)
#define STATUS_VMRESUME_AFTER_VMXOFF                                            VMX_STATUS(VMRESUME_WITH_NON_CHILD_VMCS_ERROR)
#define STATUS_VMREAD_VMWRITE_FROM_TO_UNSSUPORTED_VMCS_COMPONENT                VMX_STATUS(VMREAD_OR_VMWRITE_OF_UNSUPPORTED_COMPONENT_ERROR)
#define STATUS_VMWRITE_TO_READONLY_VMCS_COMPONENT                               VMX_STATUS(VMWRITE_OF_READ_ONLY_COMPONENT_ERROR)
#define STATUS_VMXON_EXECUTED_IN_VMX_ROOT_OPERATION                             VMX_STATUS(VMXON_IN_VMX_ROOT_OPERATION_ERROR)
#define STATUS_VMXOFF_UNDER_DUAL_MONITOR                                        VMX_STATUS(VMXOFF_WITH_CONFIGURED_SMM_MONITOR_ERROR)
#define STATUS_INVALID_OPERAND_TO_INVEPT_INVVPID                                VMX_STATUS(INVALIDATION_WITH_INVALID_OPERAND)

//
//  Configuration errors.
//
#define STATUS_VMX_NOT_SUPPORTED                                                VMX_STATUS(30)  //!< No virtualization technology present for Intel
#define STATUS_VMX_BIOS_DISABLED                                                VMX_STATUS(31)  //!< Virtualization technology disabled in BIOS
#define STATUS_VMX_DIFFERENT_CONFIG_ACROSS_PROCESSORS                                VMX_STATUS(32)  //!< Mixed environment with different kind of processors with different capabilities
#define STATUS_VMX_EPT_NOT_SUPPORTED                                            VMX_STATUS(33)  //!< Mixed environment with different kind of processors with different capabilities

//
//  VM Exits.
//
#define EXIT_REASON_SOFTWARE_INTERRUPT_EXCEPTION_NMI  0
#define EXIT_REASON_HARDWARE_INTERRUPT                1
#define EXIT_REASON_TRIPLE_FAULT                      2
#define EXIT_REASON_INIT                              3
#define EXIT_REASON_SIPI                              4
#define EXIT_REASON_IO_SMI                            5
#define EXIT_REASON_OTHER_SMI                         6
#define EXIT_REASON_PENDING_INTERRUPT                 7
#define EXIT_REASON_NMI_WINDOW                        8
#define EXIT_REASON_TASK_SWITCH                       9
#define EXIT_REASON_CPUID                             10
#define EXIT_REASON_GETSEC                            11
#define EXIT_REASON_HLT                               12
#define EXIT_REASON_INVD                              13
#define EXIT_REASON_INVLPG                            14
#define EXIT_REASON_RDPMC                             15
#define EXIT_REASON_RDTSC                             16
#define EXIT_REASON_RSM                               17
#define EXIT_REASON_VMCALL                            18
#define EXIT_REASON_VMCLEAR                           19
#define EXIT_REASON_VMLAUNCH                          20
#define EXIT_REASON_VMPTRLD                           21
#define EXIT_REASON_VMPTRST                           22
#define EXIT_REASON_VMREAD                            23
#define EXIT_REASON_VMRESUME                          24
#define EXIT_REASON_VMWRITE                           25
#define EXIT_REASON_VMXOFF                            26
#define EXIT_REASON_VMXON                             27
#define EXIT_REASON_CR_ACCESS                         28
#define EXIT_REASON_DR_ACCESS                         29
#define EXIT_REASON_IO_INSTRUCTION                    30
#define EXIT_REASON_MSR_READ                          31
#define EXIT_REASON_MSR_WRITE                         32
#define EXIT_REASON_FAILED_VMENTER_INVALID_GUEST_STATE 33
#define EXIT_REASON_FAILED_VMENTER_MSR_LOADING        34
#define EXIT_REASON_FAILED_VMEXIT                     35
#define EXIT_REASON_MWAIT_INSTRUCTION                 36
#define EXIT_REASON_MONITOR_TRAP_FLAG                 37
#define EXIT_REASON_INVALID_VMEXIT_REASON1            38
#define EXIT_REASON_MONITOR_INSTRUCTION               39
#define EXIT_REASON_PAUSE_INSTRUCTION                 40
#define EXIT_REASON_ENTRY_FAILURE_MACHINE_CHECK       41
#define EXIT_REASON_INVALID_VMEXIT_REASON2            42
#define EXIT_REASON_TPR_BELOW_THRESHOLD               43
#define EXIT_REASON_APIC_ACCESS                       44
#define EXIT_REASON_VIRTUALIZED_EOI                   45
#define EXIT_REASON_GDTR_IDTR                         46
#define EXIT_REASON_LDTR_TR                           47
#define EXIT_REASON_EPT_VIOLATION                     48
#define EXIT_REASON_EPT_MISCONFIGURATION              49
#define EXIT_REASON_INVEPT                            50
#define EXIT_REASON_RDTSCP                            51
#define EXIT_REASON_PREEMPTION_TIMER_EXPIRED          52
#define EXIT_REASON_INVVPID                           53
#define EXIT_REASON_WBINVD                            54
#define EXIT_REASON_XSETBV                            55
#define EXIT_REASON_APIC_WRITE                        56
#define EXIT_REASON_RDRAND                            57
#define EXIT_REASON_INVPCID                           58
#define EXIT_REASON_INVALID_VMFUNC                    59
#define EXIT_REASON_ENCLS                             60
#define EXIT_REASON_RDSEED                            61
#define EXIT_REASON_PML_FULL                          62
#define EXIT_REASON_XSAVES                            63
#define EXIT_REASON_XRSTORS                           64

//VMCS Fields
//Type
#define TYPE_CONTROL            0
#define TYPE_EXIT_INFORMATION   1 
#define TYPE_GUEST              2
#define TYPE_HOST               3
//Width
#define WIDTH_16            0
#define WIDTH_32            2
#define WIDTH_64            1
#define WIDTH_PLATFORM      3
//Constructor
#define VMCS_FIELD(TYPE, WIDTH, INDEX) (((WIDTH) << 13) | ((TYPE) << 10) | ((INDEX) << 1))
#define VMCS_FIELD_HIGH(FIELD) ((FIELD) + 1)
//Getters
#define VMCS_FIELD_TYPE(FIELD)  (((FIELD) & (3 << 10)) >> 10)
#define VMCS_FIELD_WIDTH(FIELD) (((FIELD) & (3 << 13)) >> 13)

//VMCS GUEST related Fields
#define GUEST_CR0                                            VMCS_FIELD(TYPE_GUEST,                WIDTH_PLATFORM,   0)
#define GUEST_CR3                                            VMCS_FIELD(TYPE_GUEST,                WIDTH_PLATFORM,   1)
#define GUEST_CR4                                            VMCS_FIELD(TYPE_GUEST,                WIDTH_PLATFORM,   2)
#define GUEST_DR7                                            VMCS_FIELD(TYPE_GUEST,                WIDTH_PLATFORM,   13)
#define GUEST_RSP                                            VMCS_FIELD(TYPE_GUEST,                WIDTH_PLATFORM,   14)
#define GUEST_RIP                                            VMCS_FIELD(TYPE_GUEST,                WIDTH_PLATFORM,   15)
#define GUEST_RFLAGS                                         VMCS_FIELD(TYPE_GUEST,                WIDTH_PLATFORM,   16)
#define GUEST_CS_SELECTOR                                    VMCS_FIELD(TYPE_GUEST,                WIDTH_16,         1)
#define GUEST_CS_BASE                                        VMCS_FIELD(TYPE_GUEST,                WIDTH_PLATFORM,   4)
#define GUEST_CS_LIMIT                                       VMCS_FIELD(TYPE_GUEST,                WIDTH_32,         1)
#define GUEST_CS_ACCESS_RIGHTS                               VMCS_FIELD(TYPE_GUEST,                WIDTH_32,         11)
#define GUEST_SS_SELECTOR                                    VMCS_FIELD(TYPE_GUEST,                WIDTH_16,         2)
#define GUEST_SS_BASE                                        VMCS_FIELD(TYPE_GUEST,                WIDTH_PLATFORM,   5)
#define GUEST_SS_LIMIT                                       VMCS_FIELD(TYPE_GUEST,                WIDTH_32,         2)
#define GUEST_SS_ACCESS_RIGHTS                               VMCS_FIELD(TYPE_GUEST,                WIDTH_32,         12)
#define GUEST_DS_SELECTOR                                    VMCS_FIELD(TYPE_GUEST,                WIDTH_16,         3)
#define GUEST_DS_BASE                                        VMCS_FIELD(TYPE_GUEST,                WIDTH_PLATFORM,   6)
#define GUEST_DS_LIMIT                                       VMCS_FIELD(TYPE_GUEST,                WIDTH_32,         3)
#define GUEST_DS_ACCESS_RIGHTS                               VMCS_FIELD(TYPE_GUEST,                WIDTH_32,         13)
#define GUEST_ES_SELECTOR                                    VMCS_FIELD(TYPE_GUEST,                WIDTH_16,         0)
#define GUEST_ES_BASE                                        VMCS_FIELD(TYPE_GUEST,                WIDTH_PLATFORM,   3)
#define GUEST_ES_LIMIT                                       VMCS_FIELD(TYPE_GUEST,                WIDTH_32,         0)
#define GUEST_ES_ACCESS_RIGHTS                               VMCS_FIELD(TYPE_GUEST,                WIDTH_32,         10)
#define GUEST_FS_SELECTOR                                    VMCS_FIELD(TYPE_GUEST,                WIDTH_16,         4)
#define GUEST_FS_BASE                                        VMCS_FIELD(TYPE_GUEST,                WIDTH_PLATFORM,   7)
#define GUEST_FS_LIMIT                                       VMCS_FIELD(TYPE_GUEST,                WIDTH_32,         4)
#define GUEST_FS_ACCESS_RIGHTS                               VMCS_FIELD(TYPE_GUEST,                WIDTH_32,         14)
#define GUEST_GS_SELECTOR                                    VMCS_FIELD(TYPE_GUEST,                WIDTH_16,         5)
#define GUEST_GS_BASE                                        VMCS_FIELD(TYPE_GUEST,                WIDTH_PLATFORM,   8)
#define GUEST_GS_LIMIT                                       VMCS_FIELD(TYPE_GUEST,                WIDTH_32,         5)
#define GUEST_GS_ACCESS_RIGHTS                               VMCS_FIELD(TYPE_GUEST,                WIDTH_32,         15)
#define GUEST_LDTR_SELECTOR                                  VMCS_FIELD(TYPE_GUEST,                WIDTH_16,         6)
#define GUEST_LDTR_BASE                                      VMCS_FIELD(TYPE_GUEST,                WIDTH_PLATFORM,   9)
#define GUEST_LDTR_LIMIT                                     VMCS_FIELD(TYPE_GUEST,                WIDTH_32,         6)
#define GUEST_LDTR_ACCESS_RIGHTS                             VMCS_FIELD(TYPE_GUEST,                WIDTH_32,         16)
#define GUEST_TR_SELECTOR                                    VMCS_FIELD(TYPE_GUEST,                WIDTH_16,         7)
#define GUEST_TR_BASE                                        VMCS_FIELD(TYPE_GUEST,                WIDTH_PLATFORM,   10)
#define GUEST_TR_LIMIT                                       VMCS_FIELD(TYPE_GUEST,                WIDTH_32,         7)
#define GUEST_TR_ACCESS_RIGHTS                               VMCS_FIELD(TYPE_GUEST,                WIDTH_32,         17)
#define GUEST_GDTR_BASE                                      VMCS_FIELD(TYPE_GUEST,                WIDTH_PLATFORM,   11)
#define GUEST_GDTR_LIMIT                                     VMCS_FIELD(TYPE_GUEST,                WIDTH_32,         8)
#define GUEST_IDTR_BASE                                      VMCS_FIELD(TYPE_GUEST,                WIDTH_PLATFORM,   12)
#define GUEST_IDTR_LIMIT                                     VMCS_FIELD(TYPE_GUEST,                WIDTH_32,         9)
#define GUEST_IA32_DEBUGCTL                                  VMCS_FIELD(TYPE_GUEST,                WIDTH_64,         1)
#define GUEST_IA32_SYSENTER_CS                               VMCS_FIELD(TYPE_GUEST,                WIDTH_32,         21)
#define GUEST_IA32_SYSENTER_ESP                              VMCS_FIELD(TYPE_GUEST,                WIDTH_PLATFORM,   18)
#define GUEST_IA32_SYSENTER_EIP                              VMCS_FIELD(TYPE_GUEST,                WIDTH_PLATFORM,   19)
#define GUEST_IA32_PERF_GLOBAL_CTRL                          VMCS_FIELD(TYPE_GUEST,                WIDTH_64,         4)
#define GUEST_IA32_PAT                                       VMCS_FIELD(TYPE_GUEST,                WIDTH_64,         2)
#define GUEST_IA32_EFER                                      VMCS_FIELD(TYPE_GUEST,                WIDTH_64,         3)
#define GUEST_IA32_BNDCFGS                                   VMCS_FIELD(TYPE_GUEST,                WIDTH_64,         9)
#define GUEST_SMBASE                                         VMCS_FIELD(TYPE_GUEST,                WIDTH_32,         20)
#define GUEST_ACTIVITY_STATE                                 VMCS_FIELD(TYPE_GUEST,                WIDTH_32,         19)
#define GUEST_INTERRUPTIBILITY_STATE                         VMCS_FIELD(TYPE_GUEST,                WIDTH_32,         18)
#define GUEST_PENDING_DEBUG_EXCEPTIONS                       VMCS_FIELD(TYPE_GUEST,                WIDTH_PLATFORM,   17)
#define VMCS_LINK_POINTER                                    VMCS_FIELD(TYPE_GUEST,                WIDTH_64,         0)
#define VMX_PREEMPTION_TIMER_VALUE                           VMCS_FIELD(TYPE_GUEST,                WIDTH_32,         23)
#define GUEST_PDPTE_0                                        VMCS_FIELD(TYPE_GUEST,                WIDTH_64,         5)
#define GUEST_PDPTE_1                                        VMCS_FIELD(TYPE_GUEST,                WIDTH_64,         6)
#define GUEST_PDPTE_2                                        VMCS_FIELD(TYPE_GUEST,                WIDTH_64,         7)
#define GUEST_PDPTE_3                                        VMCS_FIELD(TYPE_GUEST,                WIDTH_64,         8)
#define GUEST_INTERRUPT_STATUS                               VMCS_FIELD(TYPE_GUEST,                WIDTH_16,         8)
#define GUEST_PML_INDEX                                      VMCS_FIELD(TYPE_GUEST,                WIDTH_16,         9)

//VMCS HOST related fields
#define HOST_CR0                                             VMCS_FIELD(TYPE_HOST,                 WIDTH_PLATFORM,   0)
#define HOST_CR3                                             VMCS_FIELD(TYPE_HOST,                 WIDTH_PLATFORM,   1)
#define HOST_CR4                                             VMCS_FIELD(TYPE_HOST,                 WIDTH_PLATFORM,   2)
#define HOST_RSP                                             VMCS_FIELD(TYPE_HOST,                 WIDTH_PLATFORM,   10)
#define HOST_RIP                                             VMCS_FIELD(TYPE_HOST,                 WIDTH_PLATFORM,   11)
#define HOST_CS_SELECTOR                                     VMCS_FIELD(TYPE_HOST,                 WIDTH_16,         1)
#define HOST_SS_SELECTOR                                     VMCS_FIELD(TYPE_HOST,                 WIDTH_16,         2)
#define HOST_DS_SELECTOR                                     VMCS_FIELD(TYPE_HOST,                 WIDTH_16,         3)
#define HOST_ES_SELECTOR                                     VMCS_FIELD(TYPE_HOST,                 WIDTH_16,         0)
#define HOST_FS_SELECTOR                                     VMCS_FIELD(TYPE_HOST,                 WIDTH_16,         4)
#define HOST_FS_BASE                                         VMCS_FIELD(TYPE_HOST,                 WIDTH_PLATFORM,   3)
#define HOST_GS_SELECTOR                                     VMCS_FIELD(TYPE_HOST,                 WIDTH_16,         5)
#define HOST_GS_BASE                                         VMCS_FIELD(TYPE_HOST,                 WIDTH_PLATFORM,   4)
#define HOST_TR_SELECTOR                                     VMCS_FIELD(TYPE_HOST,                 WIDTH_16,         6)
#define HOST_TR_BASE                                         VMCS_FIELD(TYPE_HOST,                 WIDTH_PLATFORM,   5)
#define HOST_GDTR_BASE                                       VMCS_FIELD(TYPE_HOST,                 WIDTH_PLATFORM,   6)
#define HOST_IDTR_BASE                                       VMCS_FIELD(TYPE_HOST,                 WIDTH_PLATFORM,   7)
#define HOST_IA32_SYSENTER_CS                                VMCS_FIELD(TYPE_HOST,                 WIDTH_32,         0)
#define HOST_IA32_SYSENTER_ESP                               VMCS_FIELD(TYPE_HOST,                 WIDTH_PLATFORM,   8)
#define HOST_IA32_SYSENTER_EIP                               VMCS_FIELD(TYPE_HOST,                 WIDTH_PLATFORM,   9)
#define HOST_IA32_PERF_GLOBAL_CTRL                           VMCS_FIELD(TYPE_HOST,                 WIDTH_64,         2)
#define HOST_IA32_PAT                                        VMCS_FIELD(TYPE_HOST,                 WIDTH_64,         0)
#define HOST_IA32_EFER                                       VMCS_FIELD(TYPE_HOST,                 WIDTH_64,         1)

//VMCS CONTROL related fields
//VM Execution
#define VM_EXEC_CONTROLS_PIN_BASED                           VMCS_FIELD(TYPE_CONTROL,              WIDTH_32,         0)
#define VM_EXEC_CONTROLS_PROC_PRIMARY                        VMCS_FIELD(TYPE_CONTROL,              WIDTH_32,         1)
#define VM_EXEC_CONTROLS_PROC_SECONDARY                      VMCS_FIELD(TYPE_CONTROL,              WIDTH_32,         15)
#define EXCEPTION_BITMAP                                     VMCS_FIELD(TYPE_CONTROL,              WIDTH_32,         2)
#define PAGE_FAULT_ERRORCODE_MASK                            VMCS_FIELD(TYPE_CONTROL,              WIDTH_32,         3)
#define PAGE_FAULT_ERRORCODE_MATCH                           VMCS_FIELD(TYPE_CONTROL,              WIDTH_32,         4)
#define IO_A_BITMAP_ADDRESS                                  VMCS_FIELD(TYPE_CONTROL,              WIDTH_64,         0)
#define IO_B_BITMAP_ADDRESS                                  VMCS_FIELD(TYPE_CONTROL,              WIDTH_64,         1)
#define TSC_OFFSET                                           VMCS_FIELD(TYPE_CONTROL,              WIDTH_64,         8)
#define TSC_MULTIPLIER                                       VMCS_FIELD(TYPE_CONTROL,              WIDTH_64,         25)
#define CR0_GUEST_HOST_MASK                                  VMCS_FIELD(TYPE_CONTROL,              WIDTH_PLATFORM,   0)
#define CR0_READ_SHADOW                                      VMCS_FIELD(TYPE_CONTROL,              WIDTH_PLATFORM,   2)
#define CR4_GUEST_HOST_MASK                                  VMCS_FIELD(TYPE_CONTROL,              WIDTH_PLATFORM,   1)
#define CR4_READ_SHADOW                                      VMCS_FIELD(TYPE_CONTROL,              WIDTH_PLATFORM,   3)
#define CR3_TARGET_COUNT                                     VMCS_FIELD(TYPE_CONTROL,              WIDTH_32,         5)
#define CR3_TARGET_0                                         VMCS_FIELD(TYPE_CONTROL,              WIDTH_PLATFORM,   4)
#define CR3_TARGET_1                                         VMCS_FIELD(TYPE_CONTROL,              WIDTH_PLATFORM,   5)
#define CR3_TARGET_2                                         VMCS_FIELD(TYPE_CONTROL,              WIDTH_PLATFORM,   6)
#define CR3_TARGET_3                                         VMCS_FIELD(TYPE_CONTROL,              WIDTH_PLATFORM,   7)
#define APIC_ACCESS_ADDRESS                                  VMCS_FIELD(TYPE_CONTROL,              WIDTH_64,         10)
#define VIRTUAL_APIC_ADDRESS                                 VMCS_FIELD(TYPE_CONTROL,              WIDTH_64,         9)
#define TPR_THRESHOLD                                        VMCS_FIELD(TYPE_CONTROL,              WIDTH_32,         14)
#define EOI_EXIT_BITMAP_0                                    VMCS_FIELD(TYPE_CONTROL,              WIDTH_64,         14)
#define EOI_EXIT_BITMAP_1                                    VMCS_FIELD(TYPE_CONTROL,              WIDTH_64,         15)
#define EOI_EXIT_BITMAP_2                                    VMCS_FIELD(TYPE_CONTROL,              WIDTH_64,         16)
#define EOI_EXIT_BITMAP_3                                    VMCS_FIELD(TYPE_CONTROL,              WIDTH_64,         17)
#define POSTED_INTERRUPT_NOTIFICATION_VECTOR                 VMCS_FIELD(TYPE_CONTROL,              WIDTH_16,         1)
#define POSTED_INTERRUPT_DESCRIPTOR_ADDRESS                  VMCS_FIELD(TYPE_CONTROL,              WIDTH_64,         11)
#define MSR_BITMAP_ADDRESS                                   VMCS_FIELD(TYPE_CONTROL,              WIDTH_64,         2)
#define EXECUTIVE_VMCS_POINTER                               VMCS_FIELD(TYPE_CONTROL,              WIDTH_64,         6)
#define EPT_POINTER                                          VMCS_FIELD(TYPE_CONTROL,              WIDTH_64,         13)
#define VM_VPID                                              VMCS_FIELD(TYPE_CONTROL,              WIDTH_16,         0)
#define PLE_GAP                                              VMCS_FIELD(TYPE_CONTROL,              WIDTH_32,         16)
#define PLE_WINDOW                                           VMCS_FIELD(TYPE_CONTROL,              WIDTH_32,         17)
#define VM_FUNCTION_CONTROLS                                 VMCS_FIELD(TYPE_CONTROL,              WIDTH_64,         12)
#define EPTP_LIST_ADDRESS                                    VMCS_FIELD(TYPE_CONTROL,              WIDTH_64,         18)
#define VMREAD_BITMAP_ADDRESS                                VMCS_FIELD(TYPE_CONTROL,              WIDTH_64,         19)
#define VMWRITE_BITMAP_ADDRESS                               VMCS_FIELD(TYPE_CONTROL,              WIDTH_64,         20)
#define ENCLS_EXITING_BITMAP_ADDRESS                         VMCS_FIELD(TYPE_CONTROL,              WIDTH_64,         23)
#define PML_ADDRESS                                          VMCS_FIELD(TYPE_CONTROL,              WIDTH_64,         7)
#define VIRTUALIZATION_EXCEPTION_INFORMATION_ADDRESS         VMCS_FIELD(TYPE_CONTROL,              WIDTH_64,         21)
#define EPTP_INDEX                                           VMCS_FIELD(TYPE_CONTROL,              WIDTH_16,         2)
#define XSS_EXITING_BITMAP                                   VMCS_FIELD(TYPE_CONTROL,              WIDTH_64,         22)

//VM Exits
#define VM_EXIT_CONTROLS                                     VMCS_FIELD(TYPE_CONTROL,              WIDTH_32,         6)
#define VM_EXIT_MSR_STORE_COUNT                              VMCS_FIELD(TYPE_CONTROL,              WIDTH_32,         7)
#define VM_EXIT_MSR_STORE_ADDRESS                            VMCS_FIELD(TYPE_CONTROL,              WIDTH_64,         3)
#define VM_EXIT_MSR_LOAD_COUNT                               VMCS_FIELD(TYPE_CONTROL,              WIDTH_32,         8)
#define VM_EXIT_MSR_LOAD_ADDRESS                             VMCS_FIELD(TYPE_CONTROL,              WIDTH_64,         4)

//VM Entries
#define VM_ENTRY_CONTROLS                                    VMCS_FIELD(TYPE_CONTROL,              WIDTH_32,         9)
#define VM_ENTRY_MSR_LOAD_COUNT                              VMCS_FIELD(TYPE_CONTROL,              WIDTH_32,         10)
#define VM_ENTRY_MSR_LOAD_ADDRESS                            VMCS_FIELD(TYPE_CONTROL,              WIDTH_64,         5)
#define VM_ENTRY_INTERRUPTION_INFORMATION                    VMCS_FIELD(TYPE_CONTROL,              WIDTH_32,         11)
#define VM_ENTRY_EXCEPTION_ERRORCODE                         VMCS_FIELD(TYPE_CONTROL,              WIDTH_32,         12)
#define VM_ENTRY_INSTRUCTION_LENGTH                          VMCS_FIELD(TYPE_CONTROL,              WIDTH_32,         13)

//VMCS Exit Information
#define EXIT_REASON                                          VMCS_FIELD(TYPE_EXIT_INFORMATION,     WIDTH_32,         1)
#define EXIT_QUALIFICATION                                   VMCS_FIELD(TYPE_EXIT_INFORMATION,     WIDTH_PLATFORM,   0)
#define GUEST_LINEAR_ADDRESS                                 VMCS_FIELD(TYPE_EXIT_INFORMATION,     WIDTH_PLATFORM,   5)
#define GUEST_PHYSICAL_ADDRESS                               VMCS_FIELD(TYPE_EXIT_INFORMATION,     WIDTH_64,         0)
#define EXIT_INTERRUPTION_INFORMATION                        VMCS_FIELD(TYPE_EXIT_INFORMATION,     WIDTH_32,         2)
#define EXIT_INTERRUPTION_ERRORCODE                          VMCS_FIELD(TYPE_EXIT_INFORMATION,     WIDTH_32,         3)
#define IDT_VECTORING_INFORMATION                            VMCS_FIELD(TYPE_EXIT_INFORMATION,     WIDTH_32,         4)
#define IDT_VECTORING_ERRORCODE                              VMCS_FIELD(TYPE_EXIT_INFORMATION,     WIDTH_32,         5)
#define EXIT_INSTRUCTION_LENGTH                              VMCS_FIELD(TYPE_EXIT_INFORMATION,     WIDTH_32,         6)
#define EXIT_INSTRUCTION_INFORMATION                         VMCS_FIELD(TYPE_EXIT_INFORMATION,     WIDTH_32,         7)
#define IO_RCX                                               VMCS_FIELD(TYPE_EXIT_INFORMATION,     WIDTH_PLATFORM,   1)
#define IO_RSI                                               VMCS_FIELD(TYPE_EXIT_INFORMATION,     WIDTH_PLATFORM,   2)
#define IO_RDI                                               VMCS_FIELD(TYPE_EXIT_INFORMATION,     WIDTH_PLATFORM,   3)
#define IO_RIP                                               VMCS_FIELD(TYPE_EXIT_INFORMATION,     WIDTH_PLATFORM,   4)
#define VM_INSTRUCTION_ERROR                                 VMCS_FIELD(TYPE_EXIT_INFORMATION,     WIDTH_32,         0)
////////////
//END VMCS//
////////////

// FEATURE CONTROL MSR
#define IA32_FEATURE_CONTROL                            0x3a
#define IA32_FEATURE_CONTROL_LOCK                     0x0001
#define IA32_FEATURE_CONTROL_ENABLE_VMXON_OUTSIDE_SMX 0x0004

#define IA32_VMX_BASIC         0x480
#define IA32_VMX_CTRL_PIN      0x481
#define IA32_VMX_CTRL_CPU0     0x482
#define IA32_VMX_CTRL_EXIT     0x483
#define IA32_VMX_CTRL_ENTRY    0x484
#define IA32_VMX_CTRL_MISC     0x485
#define IA32_VMX_CR0_FIXED0    0x486
#define IA32_VMX_CR0_FIXED1    0x487
#define IA32_VMX_CR4_FIXED0    0x488
#define IA32_VMX_CR4_FIXED1    0x489
#define IA32_VMX_VMCS_ENUM     0x48a
#define IA32_VMX_CTRL_CPU1     0x48b
#define IA32_VMX_EPT_VPID      0x48c
#define IA32_VMX_TRUE_PIN      0x48d
#define IA32_VMX_TRUE_CPU0     0x48e
#define IA32_VMX_TRUE_EXIT     0x48f
#define IA32_VMX_TRUE_ENTRY    0x490

#define MEMORY_TYPE_UNCACHEABLE 0    //!< Uncacheable memory. See field BASIC_MEMORY_TYPE.
#define MEMORY_TYPE_WRITEBACK   6    //!< Writeback memory.   See field BASIC_MEMORY_TYPE.

#define EXCEPTION_ERROR_CODE_VALID  8

typedef enum _INTERRUPT_TYPE
{
    INTERRUPT_EXTERNAL             = 0,
    INTERRUPT_NMI                  = 2,
    INTERRUPT_HARDWARE_EXCEPTION   = 3,
    INTERRUPT_SOFTWARE             = 4,
    INTERRUPT_PRIVILIGED_EXCEPTION = 5,
    INTERRUPT_SOFTWARE_EXCEPTION   = 6,
    INTERRUPT_OTHER_EVENT          = 7
} INTERRUPT_TYPE;

//
//  VMX entry/exit Interrupt info
//
#define VMX_INT_INFO_VALID             (1U<<31)

//
//  Exception vectors.
//
typedef enum _VECTOR_EXCEPTION
{
    VECTOR_DIVIDE_ERROR_EXCEPTION          = 0,
    VECTOR_DEBUG_EXCEPTION                 = 1,
    VECTOR_NMI_INTERRUPT                   = 2,
    VECTOR_BREAKPOINT_EXCEPTION            = 3,
    VECTOR_OVERFLOW_EXCEPTION              = 4,
    VECTOR_BOUND_EXCEPTION                 = 5,
    VECTOR_INVALID_OPCODE_EXCEPTION        = 6,
    VECTOR_DEVICE_NOT_AVAILABLE_EXCEPTION  = 7,
    VECTOR_DOUBLE_FAULT_EXCEPTION          = 8,
    VECTOR_COPROCESSOR_SEGMENT_OVERRUN     = 9,
    VECTOR_INVALID_TSS_EXCEPTION           = 10,
    VECTOR_SEGMENT_NOT_PRESENT             = 11,
    VECTOR_STACK_FAULT_EXCEPTION           = 12,
    VECTOR_GENERAL_PROTECTION_EXCEPTION    = 13,
    VECTOR_PAGE_FAULT_EXCEPTION            = 14,
    VECTOR_X87_FLOATING_POINT_ERROR        = 16,
    VECTOR_ALIGNMENT_CHECK_EXCEPTION       = 17,
    VECTOR_MACHINE_CHECK_EXCEPTION         = 18,
    VECTOR_SIMD_FLOATING_POINT_EXCEPTION   = 19,
    VECTOR_VIRTUALIZATION_EXCEPTION        = 20
} VECTOR_EXCEPTION;

#define INTR_INFO_VECTOR_MASK           0x4F

typedef union {
    struct {
        unsigned lock:1;
        unsigned enableVmxonInSmx:1;
        unsigned enableVmxonOutsideSmx:1;
        unsigned _reserved0:5;
        unsigned senterEnables:8;
        unsigned _reserved1:16;
        unsigned _reserved2:32;
    } Bits;

    struct {
        unsigned lower;
        unsigned upper;
    } AsUint32;

    UINT64 AsUint64;
} VMX_MSR_FEATURE_CONTROL;

//
//  VMX MSR Structure - VMX_MSR_BASIC - Index 0x480
//
typedef union _VMX_MSR_BASIC
{
    struct
    {
        unsigned revisionId               : 31; // 0 - 30
        unsigned _reserved0               : 1;  // 31
        unsigned vmcsRegionSize           : 13; // 32 - 44
        unsigned _reserved1               : 3;  // 45 - 47
        unsigned physicalAddressWidth     : 1;  // 48
        unsigned dualMonitor              : 1;  // 49
        unsigned vmcsMemoryType           : 4;  // 50 - 53
        unsigned vmcsInstructionInfoValid : 1;  // 54
        unsigned defaultSettings          : 1;  // 55
        unsigned _reserved2               : 8;  // 56 - 63
    } Bits;

    UINT64 AsUint64;
} VMX_MSR_BASIC, *PVMX_MSR_BASIC;

//
//  Pin based Controls
//
#define IA32_VMX_PIN_CTLS           0x481
#define IA32_VMX_TRUE_PINBASED_CTLS 0x48D

//
//  VMX MSR Structure - IA32_MSR_PIN_BASED_VM_EXECUTION_CONTROLS_INDEX - Index 0x481
//
typedef union _VMX_PIN_EXECUTION_CTLS
{
    struct
    {
        unsigned externalInterruptExiting : 1;  // 0
        unsigned hostInterrupt           : 1;
        unsigned init                     : 1;
        unsigned nmiExiting               : 1;  // 3
        unsigned sipi                     : 1;  // 4
        unsigned virtualNmis              : 1;  // 5
        unsigned activatePreemptionTimer  : 1;  // 6
        unsigned processPostedInterrupts  : 1;  // 7
        unsigned _reserved2               : 24; // 8 - 31
    } Bits;

    UINT32 AsUint32;
} VMX_PIN_EXECUTION_CTLS;

typedef union {
    struct {
        VMX_PIN_EXECUTION_CTLS    Maybe0;
        VMX_PIN_EXECUTION_CTLS    Maybe1;
    }  Bits;

    UINT64 AsUint64;
}  VMX_PIN_EXECUTION_CTLS_MAYBE;

//
//  Processor based execution controls.
//
#define IA32_VMX_PROC_PRIMARY_CTLS      0x482
#define IA32_VMX_TRUE_PROCBASED_CTLS    0x48E

//
//  VMX MSR Structure - IA32_VMX_PROC_PRIMARY_CTLS - Index 0x482
//
typedef union _VMX_PROC_PRIMARY_CTLS
{
    struct
    {
        unsigned softwareInterrupt         : 1; // 0
        unsigned tripleFault               : 1; // 1
        unsigned interruptWindowExiting    : 1; // 2
        unsigned useTscOffseting           : 1; // 3
        unsigned taskSwitch                : 1; // 4
        unsigned cpuid                     : 1; // 5
        unsigned getSec                    : 1; // 6 
        unsigned hltExiting                : 1; // 7
        unsigned invdExiting               : 1; // 8
        unsigned invlpgExiting             : 1; // 9
        unsigned mwaitExiting              : 1; // 10
        unsigned rdpmcExiting              : 1; // 11
        unsigned rdtscExiting              : 1; // 12
        unsigned rsm                       : 1; // 13
        unsigned vmInstruction             : 1; // 14 
        unsigned cr3LoadExiting            : 1; // 15
        unsigned cr3StoreExiting           : 1; // 16
        unsigned cr3Mask                   : 1; // 17
        unsigned cr3Read_shadow            : 1; // 18
        unsigned cr8LoadExiting            : 1; // 19
        unsigned cr8StoreExiting           : 1; // 20
        unsigned useTprShadow              : 1; // 21
        unsigned nmiWindowExiting          : 1; // 22
        unsigned movDrExiting              : 1; // 23
        unsigned unconditionalIoExiting    : 1; // 24
        unsigned useIoBitmaps              : 1; // 25
        unsigned msrProtection             : 1; // 26
        unsigned monitorTrapFlag           : 1; // 27
        unsigned useMsrBitmaps             : 1; // 28
        unsigned monitorExiting            : 1; // 29
        unsigned pauseExiting              : 1; // 30
        unsigned SecondaryControls         : 1; // 31
    } Bits;
    
    UINT32 AsUint32;
} VMX_PROC_PRIMARY_CTLS;

typedef union
{
    struct
    {
        VMX_PROC_PRIMARY_CTLS Maybe0;
        VMX_PROC_PRIMARY_CTLS Maybe1;
    } Bits;

    UINT64 AsUint64;
}  VMX_PROC_PRIMARY_CTLS_MAYBE;

//
//  Secondary processor based controls.
//
#define IA32_VMX_PROC_SECONDARY_CTLS 0x48B

//
//  VMX MSR Structure - _VMX_PROC_SECONDARY_CTLS - Index 0x48B
//
typedef union _VMX_PROC_SECONDARY_CTLS
{
    struct
    {
        unsigned virtualizeApicAccess       : 1;  // 0
        unsigned enableEpt                  : 1;  // 1
        unsigned descriptorTableExiting     : 1;  // 2
        unsigned enableRdtscp               : 1;  // 3
        unsigned virtualizeX2ApicMode       : 1;  // 4
        unsigned enableVpid                 : 1;  // 5
        unsigned wbinvdExiting              : 1;  // 6
        unsigned unrestrictedGuest          : 1;  // 7
        unsigned apicRegisterVirtualization : 1;  // 8
        unsigned virtualInterruptDelivery   : 1;  // 9
        unsigned pauseLoopExiting           : 1;  // 10
        unsigned rdrandExiting              : 1;  // 11
        unsigned enableInvpcid              : 1;  // 12
        unsigned enableVmFunctions          : 1;  // 13
        unsigned vmcsShadowing              : 1;  // 14
        unsigned enableEnclsExiting         : 1;  // 15
        unsigned rdseedExiting              : 1;  // 16
        unsigned enablePml                  : 1;  // 17
        unsigned eptVe                      : 1;  // 18
        unsigned concealFromPT              : 1;  // 19
        unsigned enableXsavesXrstors        : 1;  // 20
        unsigned _reserved0                 : 1;  // 21
        unsigned modeBasedExecuteControlEpt : 1;  // 22
        unsigned _reserved1                 : 2;  // 23 - 24
        unsigned useTscScaling              : 1;  // 25
        unsigned _reserved2                 : 6;  // 26 - 31
    } Bits;

    UINT32 AsUint32;

} VMX_PROC_SECONDARY_CTLS;

typedef union
{
    struct
    {
        VMX_PROC_SECONDARY_CTLS Maybe0;
        VMX_PROC_SECONDARY_CTLS Maybe1;
    } Bits;

    UINT64 AsUint64;
}  VMX_PROC_SECONDARY_CTLS_MAYBE;

//
//  Exit controls.
//
#define IA32_VMX_EXIT_CTLS          0x483
#define IA32_VMX_TRUE_EXIT_CTLS     0x48F

//
//  VMX MSR Structure - VMX_EXIT_CTLS - Index 0x483
//

typedef union _VMX_EXIT_CTLS
{
    struct
    {
        unsigned save_cr0_and_cr4            : 1;       // 0
        unsigned save_cr3                    : 1;       // 1 
        unsigned saveDebugControls           : 1;       // 2
        unsigned save_segment_registers      : 1;       // 3
        unsigned save_esp_eip_eflags         : 1;       // 4 
        unsigned savePendingDebugExceptions  : 1;       // 5
        unsigned saveInterruptibilityInformation : 1;   // 6
        unsigned saveActivityState           : 1;       // 7
        unsigned saveWorkingVmcsPointer      : 1;       // 8
        unsigned hostAddressSpaceSize        : 1;       // 9
        unsigned load_cr0_and_cr4            : 1;       // 10
        unsigned load_cr3                    : 1;       // 11
        unsigned loadIa32PerfGlobalControl   : 1;       // 12
        unsigned loadSegmentRegisters        : 1;       // 13
        unsigned load_esp_eip                : 1;       // 14
        unsigned acknowledgeInterruptOnExit  : 1;       // 15
        unsigned saveSysEnterMsrs            : 1;       // 16
        unsigned loadSysEnterMsrs            : 1;       // 17
        unsigned saveIa32Pat                 : 1;       // 18
        unsigned loadIa32Pat                 : 1;       // 19
        unsigned saveIa32Efer                : 1;       // 20
        unsigned loadIa32Efer                : 1;       // 21
        unsigned saveVmxPreemptionTimerValue : 1;       // 22
        unsigned clearIa32Bndcgfs            : 1;       // 23
        unsigned concealVmExitsFromPT        : 1;       // 24
        unsigned _reserved0                  : 7;       // 25 - 31
    } Bits;
    
    UINT32 AsUint32;

} VMX_EXIT_CTLS, *PVMX_EXIT_CTLS;

typedef union
{
    struct
    {
        VMX_EXIT_CTLS Maybe0;
        VMX_EXIT_CTLS Maybe1;
    } Bits;

    UINT64 AsUint64;
}  VMX_EXIT_CTLS_MAYBE;

//
//  Entry controls.
//
#define IA32_VMX_ENTRY_CTLS         0x484
#define IA32_VMX_TRUE_ENTRY_CTLS    0x490

//
//  VMX MSR Structure - VMX_ENTRY_CTLS - Index 0x484
//
typedef union _VMX_ENTRY_CTLS
{
    struct
    {
        unsigned load_cr0_and_cr4           : 1;  // 0 
        unsigned load_cr3                   : 1;  // 1 
        unsigned loadDebugControls          : 1;  // 2
        unsigned loadSegmentRegisters       : 1;  // 3
        unsigned load_esp_eip_eflags        : 1;  // 4
        unsigned loadPendingDebugExceptions : 1;  //5
        unsigned loadInterruptibilityInformation : 1;  //6
        unsigned loadActivityState          : 1;  // 7
        unsigned loadWorkingVmcsPointer     : 1;  // 8
        unsigned ia32eModeGuest             : 1;  // 9
        unsigned entryToSmm                 : 1;  // 10
        unsigned deactivateDualMonitor      : 1;  // 11
        unsigned loadSysEnterMsrs           : 1;  // 12
        unsigned loadIa32PerfGlobalControl  : 1;  // 13
        unsigned loadIa32Pat                : 1;  // 14
        unsigned loadIa32Efer               : 1;  // 15
        unsigned loadIa32Bndcgfs            : 1;  // 16
        unsigned concealVmEntriesFromPT     : 1;  // 17
        unsigned _reserved30                : 14; // 18 - 31
    } Bits;

    UINT32 AsUint32;

} VMX_ENTRY_CTLS, *PVMX_ENTRY_CTLS;

typedef union
{
    struct
    {
        VMX_ENTRY_CTLS Maybe0;
        VMX_ENTRY_CTLS Maybe1;
    } Bits;

    UINT64 AsUint64;
}  VMX_ENTRY_CTLS_MAYBE;


//
//  VMX Miscellaneous
//
#define IA32_VMX_MISC 0x485

//
//  VMX MSR Structure - IA32_MSR_MISCELLANEOUS_DATA_INDEX - Index 0x485
//

typedef union _VMX_MISC
{
    struct
    {
        unsigned preemptionTimerLength             : 5;  // 0 - 4
        unsigned unrestrictedGuest                 : 1;  // 5
        unsigned entryInHaltStateSupported         : 1;  // 6
        unsigned entryInShutdownStateSupported     : 1;  // 7
        unsigned entryInWaitForSipiStateSupported  : 1;  // 8
        unsigned _reserved0                        : 6;  // 9 - 14
        unsigned canReadIa32SmbaseFromSmm          : 1;  // 15
        unsigned numberOfCr3TargetValues           : 9;  // 16 - 24
        unsigned msrListsMaxSize                   : 3;  // 25 - 27
        unsigned canReadIa32SmmMonitorControl      : 1;  // 28
        unsigned canWriteExitInfoFields            : 1;  // 29
        unsigned _reserved1                        : 2;  // 30 - 31
        unsigned msegRevisionId                    : 32; // 32 - 63
    } Bits;

    UINT64 AsUint64;

} VMX_MISC_DATA, *PVMX_MISC_DATA;
//
//  VMX EPT and VPID Capabilities.
//
#define IA32_VMX_EPT_VPID_CAP 0x48C

//
//  VMX MSR Structure - VMX_EPT_VPID_CAP - Index 0x48C
//

typedef union _VMX_EPT_VPID_CAP
{
    struct
    {
        /* RWX support */
        unsigned x_only                                   : 1;
        unsigned w_only                                   : 1;
        unsigned w_and_x_only                             : 1;
        /* gaw support */
        unsigned gaw_21_bit                               : 1;
        unsigned gaw_30_bit                               : 1;
        unsigned gaw_39_bit                               : 1;
        unsigned gaw_48_bit                               : 1;
        unsigned gaw_57_bit                               : 1;
        /* EMT support */
        unsigned uc                                       : 1;
        unsigned wc                                       : 1;
        unsigned _reserved0                               : 2;
        unsigned wt                                       : 1;
        unsigned wp                                       : 1;
        unsigned wb                                       : 1;
        unsigned reserved1                                : 1;
        /* SP support */
        unsigned sp_21_bit                                : 1;
        unsigned sp_30_bit                                : 1;
        unsigned sp_39_bit                                : 1;
        unsigned sp_48_bit                                : 1;
        unsigned supportInvept                            : 1;  // 20
        unsigned supportDirtyAccessedBits                 : 1;  // 21
        unsigned _reserved1                               : 2;  // 22 - 23
        unsigned SupportInveptIndividualAddress           : 1;  // 24
        unsigned supportInveptSingleContext               : 1;  // 25
        unsigned supportInveptAllContext                  : 1;  // 26
        unsigned _reserved2                               : 5;  // 27 - 31
        unsigned supportInvvpid                           : 1;  // 32
        unsigned _reserved3                               : 7;  // 33 - 39
        unsigned supportInvvpidIndividualAddress          : 1;  // 40
        unsigned supportInvvpidSingleContext              : 1;  // 41
        unsigned supportInvvpidAllContext                 : 1;  // 42
        unsigned supportInvvpidSingleContextRetainGlobals : 1;  // 43
        unsigned _reserved4                               : 20; // 44 - 63
    } Bits;

    UINT64 AsUint64;

} VMX_EPT_VPID_CAP;


//
//  VMX MSR Structure - VMX_VMFUNC_CTL - Index 0x491
//
typedef union
{
    struct
    {
        unsigned eptpSwitching  : 1;
        unsigned _reserved0     : 31;
        unsigned _reserved1     : 32;
    }  Bits;

    UINT64 AsUint64;

} VMX_VMFUNC_CTL;

//
//  VMX MSR Structure - IA32_MSR_CR0_ALLOWED_ZERO_INDEX,
//  IA32_MSR_CR0_ALLOWED_ONE_INDEX - Index 0x486, 0x487
//
#define IA32_VMX_CR0_FIXED0 0x486
#define IA32_VMX_CR0_FIXED1 0x487

//
// VMX MSR Structure - IA32_MSR_CR4_ALLOWED_ZERO_INDEX,
// IA32_MSR_CR4_ALLOWED_ONE_INDEX - Index 0x488, 0x489
//
#define IA32_VMX_CR4_FIXED0 0x488
#define IA32_VMX_CR4_FIXED1 0x489

//////////////////////////
//GUEST ADDITIONAL STATE// //24.4.2
//////////////////////////
typedef struct _EXIT_GUEST_INTERRUPTIBILITY_STATE
{
    union
    {
        UINT32 raw;

        struct
        {
            unsigned blockingBySti       : 1;  // 0
            unsigned blockingByMovSs     : 1;  // 1
            unsigned blockingBySmi       : 1;  // 2
            unsigned blockingByNmi       : 1;  // 3
            unsigned enclaveInterruption : 1;  // 4
            unsigned _reserved           : 27;  // 5 - 31
        } f;
    } u;
} EXIT_GUEST_INTERRUPTIBILITY_STATE, *PEXIT_GUEST_INTERRUPTIBILITY_STATE;
//////////////////////////////
//END GUEST ADDITIONAL STATE//
//////////////////////////////


///////////////////////
//EXIT QUALIFICATIONS// //27.2.1
///////////////////////
typedef struct _EXIT_QUALIFICATION_DEBUG_EXCEPTION
{
    union
    {
        UINT_PTR raw;

        struct
        {
            unsigned B0              : 1;  // 0
            unsigned B1              : 1;  // 1
            unsigned B2              : 1;  // 2
            unsigned B3              : 1;  // 3
            unsigned _reserved0      : 9;  // 4 - 12
            unsigned BD              : 1;  // 13 
            unsigned BS              : 1;  // 14 
            unsigned _reserved2      : 17;  // 15 - 31
#ifdef _WIN64
            unsigned _reserved3      : 32; // 32 - 63
#endif
        } f;
    } u;
} EXIT_QUALIFICATION_DEBUG_EXCEPTION, *PEXIT_QUALIFICATION_DEBUG_EXCEPTION;

typedef struct _EXIT_QUALIFICATION_TASK_SWITCH
{
    union
    {
        UINT_PTR raw;

        struct
        {
            unsigned tssSelector     : 16;  // 0  - 15
            unsigned _reserved0      : 14;  // 16 - 29
            unsigned source          : 2;   // 30 - 31 
#ifdef _WIN64
            unsigned _reserved1      : 32; // 32 - 63
#endif
        } f;
    } u;
} EXIT_QUALIFICATION_TASK_SWITCH, *PEXIT_QUALIFICATION_TASK_SWITCH;

#define TASK_SWITCH_SOURCE_CALL             0
#define TASK_SWITCH_SOURCE_IRET             1
#define TASK_SWITCH_SOURCE_JMP              2
#define TASK_SWITCH_SOURCE_IDT_TASK_GATE    3

typedef struct _EXIT_QUALIFICATION_CR
{
    union
    {
        UINT_PTR raw;

        struct
        {
            unsigned number          : 4;  // 0 - 3
            unsigned accessType      : 2;  // 4 - 5
            unsigned lmswOperandType : 1;  // 6 
            unsigned _reserved0      : 1;  // 7 
            unsigned moveGpr         : 4;  // 8 - 11
            unsigned _reserved1      : 4;  // 12 - 15
            unsigned lmswSourceData  : 16; // 16 - 31
#ifdef _WIN64
            unsigned _reserved2      : 32; // 32 - 63
#endif
        } cr;
    } u;
} EXIT_QUALIFICATION_CR, *PEXIT_QUALIFICATION_CR;

#define CR_ACCESS_TYPE_MOV_TO_CR    0
#define CR_ACCESS_TYPE_MOV_FROM_CR  1
#define CR_ACCESS_TYPE_CLTS         2
#define CR_ACCESS_TYPE_LMSW         3

#define CR_ACCESS_TYPE_LMSW_OPERAND_REGISTER    0
#define CR_ACCESS_TYPE_LMSW_OPERAND_MEMORY      1

typedef struct _EXIT_QUALIFICATION_DR
{
    union
    {
        UINT_PTR raw;

        struct
        {
            unsigned dr         : 3;  // 0 - 2
            unsigned _reserved0 : 1;  // 3
            unsigned accessType : 1;  // 4
            unsigned _reserved1 : 3;  // 5 - 7
            unsigned gpr        : 4;  // 8 - 11
            unsigned _reserved2 : 20; // 12 - 31
#ifdef _WIN64
            unsigned _reserved3 : 32; // 32 - 63
#endif
        } f;
    } u;
} EXIT_QUALIFICATION_DR, *PEXIT_QUALIFICATION_DR;

#define DR_ACCESS_TYPE_MOV_TO_DR    0
#define DR_ACCESS_TYPE_MOV_FROM_DR  1

typedef struct _EXIT_QUALIFICATION_IO
{
    union
    {
        UINT_PTR raw;

        struct
        {
            unsigned size                : 3;  // 0 - 2
            unsigned direction           : 1;  // 3
            unsigned isStringInstruction : 1;  // 4 
            unsigned hasRepPrefix        : 1;  // 5 
            unsigned operandEncoding     : 1;  // 6
            unsigned _reserved0          : 9;  // 7 - 15
            unsigned port                : 16; // 16 - 31
#ifdef _WIN64
            unsigned _reserved1          : 32; // 32 - 63
#endif
        } f;
    } u;
} EXIT_QUALIFICATION_IO, *PEXIT_QUALIFICATION_IO;

typedef struct _EXIT_QUALIFICATION_APIC
{
    union
    {
        UINT_PTR raw;

        struct
        {
            unsigned apicPageOffset    : 12;  // 0  - 11
            unsigned accessType        : 4;   // 12 - 15
            unsigned _reserved1        : 16;  // 16 - 31
#ifdef _WIN64
            unsigned _reserved2        : 32; // 32 - 63
#endif
        } f;
    } u;
} EXIT_QUALIFICATION_APIC, *PEXIT_QUALIFICATION_APIC;

#define APIC_ACCESS_LINEAR_FOR_READ             0
#define APIC_ACCESS_LINEAR_FOR_WRITE            1
#define APIC_ACCESS_LINEAR_FOR_EXECUTE          2
#define APIC_ACCESS_LINEAR_RW_IN_EVENT          3
#define APIC_ACCESS_GUEST_PHYSICAL_RW_IN_EVENT  10
#define APIC_ACCESS_GUEST_PHYSICAL_FOR_EXECUTE  15

typedef struct _EXIT_QUALIFICATION_SLAT
{
    union
    {
        UINT_PTR raw;

        struct
        {
            unsigned read              : 1;  // 0
            unsigned write             : 1;  // 1
            unsigned execute           : 1;  // 2
            unsigned couldRead         : 1;  // 3
            unsigned couldWrite        : 1;  // 4
            unsigned couldExecute      : 1;  // 5
            unsigned _reserved0        : 1;  // 6
            unsigned guestLinearValid  : 1;  // 7
            unsigned notPagingAccess   : 1;  // 8
            unsigned _reserved1        : 3;  // 9 - 11
            unsigned nmiUnblocking     : 1;  // 12
            unsigned _reserved2        : 19; // 13 - 31
#ifdef _WIN64
            unsigned _reserved3        : 32; // 32 - 63
#endif
        } f;
    } u;
} EXIT_QUALIFICATION_SLAT, *PEXIT_QUALIFICATION_SLAT;

//27.2.2
typedef struct _INTERRUPT_INFORMATION
{
    union
    {
        UINT32 raw;

        struct
        {
            unsigned vector        : 8;  // 0 - 7
            unsigned type          : 3;  // 8 - 10
            unsigned deliverCode   : 1;  // 11
            unsigned nmiUnblocking : 1;  // 12
            unsigned _reserved0    : 18; // 13 - 30
            unsigned valid         : 1;  // 31
        } f;
    } u;
} INTERRUPT_INFORMATION, *PINTERRUPT_INFORMATION;

#define INTERRUPT_TYPE_EXTERNAL             0
#define INTERRUPT_TYPE_NMI                  2
#define INTERRUPT_TYPE_HARDWARE_EXCEPTION   3
#define INTERRUPT_TYPE_SOFTWARE_EXCEPTION   6 //#BP - INT3  #OF - INTO

////////////////////////////
// END EXIT QUALIFICATIONS//
////////////////////////////

///////////////////////////
//INSTRUCTION INFORMATION// //27.2.4
///////////////////////////
typedef struct _INSTRUCTION_INFORMATION_IO
{
    union
    {
        UINT32 raw;

        struct
        {
            unsigned _reserved0  : 7;  // 0 - 6
            unsigned addressSize : 3;  // 7 - 9
            unsigned _reserved1  : 5;  // 10 - 14
            unsigned segment     : 3;  // 15 - 17
            unsigned _reserved2  : 14; // 18 - 31
        } f;
    } u;
} INSTRUCTION_INFORMATION_IO, *PINSTRUCTION_INFORMATION_IO;

typedef struct _INSTRUCTION_INFORMATION_SLAT
{
    union
    {
        UINT32 raw;

        struct
        {
            unsigned scaling      : 2;  // 0  - 1
            unsigned _reserved0   : 5;  // 2  - 6
            unsigned addressSize  : 2;  // 7  - 9
            unsigned _reserved1   : 5;  // 10 - 14
            unsigned segment      : 3;  // 15 - 17
            unsigned index        : 4;  // 18 - 21
            unsigned indexInvalid : 1;  // 22
            unsigned base         : 4;  // 23 - 26
            unsigned baseInvalid  : 1;  // 27
            unsigned reg2         : 1;  // 28 - 31
        } f;
    } u;
} INSTRUCTION_INFORMATION_SLAT, *PINSTRUCTION_INFORMATION_SLAT;

//IDT / GDT
typedef struct _INSTRUCTION_INFORMATION_XDT
{
    union
    {
        UINT32 raw;

        struct
        {
            unsigned scaling      : 2; // 0 - 1
            unsigned _reserved0   : 5; // 2 - 6
            unsigned addressSize  : 3; // 7 - 9
            unsigned _zero        : 1; // 10
            unsigned operandSize  : 1; // 11
            unsigned _reserved1   : 3; // 12 - 14
            unsigned segment      : 3; // 15 - 17
            unsigned index        : 4; // 18 - 21
            unsigned indexInvalid : 1; // 22
            unsigned base         : 4; // 23 - 26
            unsigned baseInvalid  : 1; // 27
            unsigned instruction  : 2; // 28 - 29
            unsigned _reserved2   : 2; // 30 - 31
        } f;
    } u;
} INSTRUCTION_INFORMATION_XDT, *PINSTRUCTION_INFORMATION_XDT;

//LDT / TR
typedef struct _INSTRUCTION_INFORMATION_XTR
{
    union
    {
        UINT32 raw;

        struct
        {
            unsigned scaling      : 2; // 0 - 1
            unsigned _reserved0   : 1; // 2
            unsigned reg1         : 4; // 3 - 6
            unsigned addressSize  : 3; // 7 - 9
            unsigned memReg       : 1; // 10
            unsigned _reserved1   : 4; // 11 - 14
            unsigned segment      : 3; // 15 - 17
            unsigned index        : 4; // 18 - 21
            unsigned indexInvalid : 1; // 22
            unsigned base         : 4; // 23 - 26
            unsigned baseInvalid  : 1; // 27
            unsigned instruction  : 2; // 28 - 29
            unsigned _reserved2   : 2; // 30 - 31
        } f;
    } u;
} INSTRUCTION_INFORMATION_XTR, *PINSTRUCTION_INFORMATION_XTR;

typedef struct _INSTRUCTION_INFORMATION_RD
{
    union
    {
        UINT32 raw;

        struct
        {
            unsigned _reserved0   : 3;  // 0  - 2
            unsigned destination  : 4;  // 3  - 6
            unsigned _reserved1   : 4;  // 7  - 10
            unsigned operandSize  : 2;  // 11 - 12
            unsigned _reserved2   : 19; // 13 - 31
        } f;
    } u;
} INSTRUCTION_INFORMATION_RD, *PINSTRUCTION_INFORMATION_RD;


#define INSTRUCTION_INFORMATION_SCALING_BY_0 0
#define INSTRUCTION_INFORMATION_SCALING_BY_2 1
#define INSTRUCTION_INFORMATION_SCALING_BY_4 2
#define INSTRUCTION_INFORMATION_SCALING_BY_8 3

#define INSTRUCTION_INFORMATION_ADDRESS_SIZE_16 0
#define INSTRUCTION_INFORMATION_ADDRESS_SIZE_32 1
#define INSTRUCTION_INFORMATION_ADDRESS_SIZE_64 2

#define INSTRUCTION_INFORMATION_OPERAND_SIZE_16 0
#define INSTRUCTION_INFORMATION_OPERAND_SIZE_32 1
#define INSTRUCTION_INFORMATION_OPERAND_SIZE_64 2

#define INSTRUCTION_INFORMATION_SEGMENT_ES 0
#define INSTRUCTION_INFORMATION_SEGMENT_CS 1
#define INSTRUCTION_INFORMATION_SEGMENT_SS 2
#define INSTRUCTION_INFORMATION_SEGMENT_DS 3
#define INSTRUCTION_INFORMATION_SEGMENT_FS 4
#define INSTRUCTION_INFORMATION_SEGMENT_GS 5

#define INSTRUCTION_INFORMATION_REG_RAX 0
#define INSTRUCTION_INFORMATION_REG_RCX 1
#define INSTRUCTION_INFORMATION_REG_RDX 2
#define INSTRUCTION_INFORMATION_REG_RBX 3
#define INSTRUCTION_INFORMATION_REG_RSP 4
#define INSTRUCTION_INFORMATION_REG_RBP 5
#define INSTRUCTION_INFORMATION_REG_RSI 6
#define INSTRUCTION_INFORMATION_REG_RDI 7
#define INSTRUCTION_INFORMATION_REG_R8  8
#define INSTRUCTION_INFORMATION_REG_R9  9
#define INSTRUCTION_INFORMATION_REG_R10 10
#define INSTRUCTION_INFORMATION_REG_R11 11
#define INSTRUCTION_INFORMATION_REG_R12 12
#define INSTRUCTION_INFORMATION_REG_R13 13
#define INSTRUCTION_INFORMATION_REG_R14 14
#define INSTRUCTION_INFORMATION_REG_R15 15

#define INSTRUCTION_INFORMATION_IDENTITY_SGDT 0
#define INSTRUCTION_INFORMATION_IDENTITY_SIDT 1
#define INSTRUCTION_INFORMATION_IDENTITY_LGDT 2
#define INSTRUCTION_INFORMATION_IDENTITY_LIDT 3

#define INSTRUCTION_INFORMATION_IDENTITY_SLDT 0
#define INSTRUCTION_INFORMATION_IDENTITY_STR  1
#define INSTRUCTION_INFORMATION_IDENTITY_LLDT 2
#define INSTRUCTION_INFORMATION_IDENTITY_LTR  3
////////////////////////////////
// END INSTRUCTION INFORMATION//
////////////////////////////////


////////////////////////////
//     EPT STRUCTURES     //
////////////////////////////
#pragma warning( disable : 4214 ) //Bitfield type distinct from int

//24.6.11
typedef struct _EPTP
{
    union
    {
        UINT64 raw;

        struct
        {
            UINT64 memoryType          : 3;   // 0 - 2
            UINT64 pageWalkLength      : 3;   // 3 - 5
            UINT64 enableAccessedDirty : 1;   // 6 
            UINT64 _reserved0          : 5;   // 7 - 11 
            UINT64 pml4Address         : 40;  // 12 - 51
            UINT64 _reserved1          : 12;  // 52 - 63
        } f;
    } u;
} EPTP, *PEPTP;

//28.2.2
typedef struct _EPT_PML4E
{
    union
    {
        UINT64 raw;

        struct
        {
            UINT64 r          : 1;  //0
            UINT64 w          : 1;  //1
            UINT64 x          : 1;  //2
            UINT64 _reserved0 : 5;  //3-7 - Must be 0
            UINT64 a          : 1;  //8
            UINT64 _ignored0  : 1;  //9
            UINT64 xu         : 1;  //10
            UINT64 _ignored1  : 1;  //11
            UINT64 address    : 40; //12-51
            UINT64 _ignored2  : 12; //52-63
        } f;
    } u;
} EPT_PML4E, *PEPT_PML4E;

typedef struct _EPT_PDPTE
{
    union
    {
        UINT64 raw;

        struct
        {
            UINT64 r          : 1;  //0
            UINT64 w          : 1;  //1
            UINT64 x          : 1;  //2
            UINT64 mt         : 3;  //3-5
            UINT64 ipat       : 1;  //6
            UINT64 b          : 1;  //7
            UINT64 a          : 1;  //8
            UINT64 d          : 1;  //9
            UINT64 xu         : 1;  //10
            UINT64 _ignored0  : 1;  //11
            UINT64 _reserved0 : 18; //12-29 - Must be 0
            UINT64 address    : 22; //30-51
            UINT64 _ignored1  : 11; //52-62
            UINT64 sve        : 1;  //63
        } b;

        struct
        {
            UINT64 r          : 1;  //0
            UINT64 w          : 1;  //1
            UINT64 x          : 1;  //2
            UINT64 _reserved0 : 4;  //3-6 - Must be 0
            UINT64 b          : 1;  //7
            UINT64 a          : 1;  //8
            UINT64 _ignored0  : 1;  //9
            UINT64 xu         : 1;  //10
            UINT64 _ignored1  : 1;  //11
            UINT64 address    : 40; //12-51
            UINT64 _ignored2  : 12; //52-63
        } nb;
    } u;
} EPT_PDPTE, *PEPT_PDPTE;

typedef struct _EPT_PDE
{
    union
    {
        UINT64 raw;

        struct
        {
            UINT64 r          : 1;  //0
            UINT64 w          : 1;  //1
            UINT64 x          : 1;  //2
            UINT64 mt         : 3;  //3-5
            UINT64 ipat       : 1;  //6
            UINT64 b          : 1;  //7
            UINT64 a          : 1;  //8
            UINT64 d          : 1;  //9
            UINT64 xu         : 1;  //10
            UINT64 _ignored0  : 1;  //11
            UINT64 _reserved0 : 1;  //12-20 - Must be 0
            UINT64 address    : 31; //21-51
            UINT64 _ignored1  : 11; //52-62
            UINT64 sve        : 1;  //63
        } b;

        struct
        {
            UINT64 r          : 1;  //0
            UINT64 w          : 1;  //1
            UINT64 x          : 1;  //2
            UINT64 _reserved0 : 4;  //3-6 - Must be 0
            UINT64 b          : 1;  //7
            UINT64 a          : 1;  //8
            UINT64 _ignored0  : 1;  //9
            UINT64 xu         : 1;  //10
            UINT64 _ignored1  : 1;  //11
            UINT64 address    : 40; //12-51
            UINT64 _ignored2  : 12; //52-63
        } nb;
    } u;
} EPT_PDE, *PEPT_PDE;

typedef struct _EPT_PTE
{
    union
    {
        UINT64 raw;

        struct
        {
            UINT64 r         : 1;  //0
            UINT64 w         : 1;  //1
            UINT64 x         : 1;  //2
            UINT64 mt        : 3;  //3-5
            UINT64 ipat      : 1;  //6
            UINT64 _ignored0 : 1;  //7
            UINT64 a         : 1;  //8
            UINT64 d         : 1;  //9
            UINT64 xu        : 1;  //10
            UINT64 _ignored1 : 1;  //11
            UINT64 address   : 40; //12-51
            UINT64 _ignored2 : 11; //52-62
            UINT64 sve       : 1;  //63
        } f;
    } u;
} EPT_PTE, *PEPT_PTE;

#pragma pack(push, 1)
typedef struct _VE_INFO_AREA
{
    UINT32 exitReason;
    UINT32 reset;
    UINT64 exitQualification;
    UINT64 guestLinearAddress;
    UINT64 guestPhysicalAddress;
    UINT16 eptpIndex;
} VE_INFO_AREA, *PVE_INFO_AREA;
#pragma pack(pop)
////////////////////////////
//  END EPT STRUCTURES    //
////////////////////////////

typedef enum _INVPID_TYPE
{
    INV_INDIV_ADDR = 0,                     // Invalidate a specific page
    INV_SINGLE_CONTEXT = 1,                 // Invalidate one context (specific VPID)
    INV_ALL_CONTEXTS = 2,                   // Invalidate all contexts (all VPIDs)
    INV_SINGLE_CONTEXT_RETAIN_GLOBALS = 3   // Invalidate a single VPID context retaining global mappings
} IVVPID_TYPE;

typedef struct _VPID_CTX
{
    ULONG64 VPID     : 16;      // VPID to effect
    ULONG64 Reserved : 48;      // Reserved
    ULONG64 Address  : 64;      // Linear address
} VPID_CTX, *PVPID_CTX;

/**
* Indicates if current processor supports VMX operation mode.
*/
NTSTATUS
VmxIsSupported(
    VOID
);

/**
* Indicates if current processor is an Intel.
*/
BOOLEAN
VmxIsIntel(
    VOID
);

/**
* Reads one of config MSR's (IA32_VMX_BASIC, IA32_VMX_MISC or IA32_VMX_EPT_VPID_CAP)
* in a safe way taking into account dependencies as some MSR may not exists.
* (eg: to be able to read IA32_VMX_EPT_VPID_CAP, bit 31 of MSR IA32_VMX_PROCBASED_CTLS must be 1
* and then bits 1 or 5 of MSR IA32_VMX_PROCBASED_CTLS2 must be set).
*/
UINT64
VmxCapability(
    _In_ UINT32 capability
);

/**
* Reads one of control MSR's (IA32_VMX_PINBASED_CTLS, IA32_VMX_PROCBASED_CTLS, IA32_VMX_PROCBASED_CTLS2, IA32_VMX_TRUE_CTLS, IA32_VMX_ENTRY_CTLS)
* in a safe way taking into account dependencies as some MSR may not exists or be superseeded by their TRUE msr counterparts.
* (eg: If bit 55 of IA32_VMX_BASIC msr is set, then control msr's index change to TRUE control MSR's).
*/
UINT32
VmxAllowed(
    _In_ UINT32 control
);

/**
* Reads a field from the VMCS. All VMCS fields index are 32 bits. Data might be 16, 32 or 64 bit wide.
*/
UINT16
VmxVmcsRead16(
    _In_ UINT32 field
);
UINT32
VmxVmcsRead32(
    _In_ UINT32 field
);
UINT64
VmxVmcsRead64(
    _In_ UINT32 field
);
UINT_PTR
VmxVmcsReadPlatform(
    _In_ UINT32 field
);

/**
* Writes a field to the VMCS. Data can be 16, 32 or 64. In 32 bit systems 64 data writes are performed with to writes.
* This also holds true in 64 although in 64 the write could be done in one step.
*/
BOOLEAN
VmxVmcsWrite16(
    _In_ UINT32 field,
    _In_ UINT16 data
);
BOOLEAN
VmxVmcsWrite32(
    _In_ UINT32 field,
    _In_ UINT32 data
);
BOOLEAN
VmxVmcsWrite64(
    _In_ UINT32 field,
    _In_ UINT64 data
);
BOOLEAN
VmxVmcsWritePlatform(
    _In_ UINT32   field,
    _In_ UINT_PTR data
);


/**
* High level vmcall. Performs a vmcall with 64-bit calling convention and 
* semantics on parameters (registers). Notice that hypervisor must implement
* the desired functionality.
*
* @param service [in] Service to be invoked in a hypervisor. 
* @param data    [in] Additional data required to perform the operation
* @return Status of the operation.
*/
NTSTATUS __stdcall
VmxCall(
    _In_ UINT_PTR service,
    _In_ UINT_PTR data
);

/**
* vmfunc call. Currently only service 0 (change ept's) is supported as this is provided
* by the micro. Additonal data in this case is an index specifying
* desired ept's. ABI is rax = service, rcx = data
*
* @param service [in] Service to be invoked in a hypervisor via vmfunc. 
* @param data    [in] Additional data required to perform the operation.
* @return Status of the operation.
*/
NTSTATUS __stdcall
VmxFunc(
    _In_ UINT_PTR service,
    _In_ UINT_PTR data
);

/**
* VMX Instructions: (we have only intrinsics for 64 bit systems)
* Return values:
*  - 0 = The operation succeeded.
*  - 1 = The operation failed with extended status available in the VM - instruction error field of the current VMCS.
*  - 2 = The operation failed without status available.
*/
#ifndef _WIN64

UINT8 __stdcall VmxOn     (PUINT64 vmxOnAddress);
#define VmxOff __vmx_off
UINT8 __stdcall VmxVmClear(PUINT64 vmcsAddress);
UINT8 __stdcall VmxVmPtrLd(PUINT64 vmcsAddress);
UINT8 __stdcall VmxVmPtrSt(PUINT64 vmcsAddress);
UINT8 __stdcall VmxVmRead (UINT32 field, PUINT_PTR data);
UINT8 __stdcall VmxVmWrite(UINT32 field, UINT_PTR  data);
UINT8 __stdcall VmxInvEpt (UINT_PTR type, PVOID eptPointer);
UINT8 __stdcall VmxInvVpid(IVVPID_TYPE type, PVPID_CTX vpidContext );

#else

#define VmxOn       __vmx_on
#define VmxOff      __vmx_off
#define VmxVmClear  __vmx_vmclear
#define VmxVmPtrLd  __vmx_vmptrld
#define VmxVmPtrSt  __vmx_vmptrst
#define VmxVmRead   __vmx_vmread
#define VmxVmWrite  __vmx_vmwrite
UINT8 VmxInvEpt(_In_ UINT_PTR type, _In_ PVOID eptPointer);
UINT8 VmxInvVpid(_In_ IVVPID_TYPE type, _In_ PVPID_CTX vpidContext );

#endif

VOID
InjectUndefinedOpcodeException(
    VOID
    );

VOID
InjectHardwareException(
    _In_ INTERRUPT_INFORMATION InterruptInfo
    );

#endif
