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
// VMX NTSTATUS Errors
//
#define VMX_STATUS_MASK  0xEB1E0000
#define VMX_STATUS(X)    ((NTSTATUS)(VMX_STATUS_MASK | ((X) & 0x0000FFFF)))

//
//  VMX Error Codes
//
#define NO_INSTRUCTION_ERROR                        0                  /* VMxxxxx */
#define VMCALL_IN_VMX_ROOT_ERROR                    1                  /* VMCALL */
#define VMCLEAR_WITH_INVALID_ADDR                   2                  /* VMCLEAR */
#define VMCLEAR_WITH_VMXON_VMCS_PTR                 3                  /* VMCLEAR */
#define VMLAUNCH_WITH_NON_CLEAR_VMCS_ERROR          4                  /* VMLAUNCH */
#define VMRESUME_WITH_NON_LAUNCHED_VMCS_ERROR       5                  /* VMRESUME */
#define VMRESUME_VMCS_CORRUPTED                     6                  /* VMRESUME */
#define VMENTER_BAD_CONTROL_FIELD_ERROR             7                  /* VMENTER */
#define VMENTER_INVALID_VM_HOST_STATE               8                  /* VMENTER */
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
#define STATUS_VM_ENTRY_WITH_INVALID_HOST_STATE_FIELDS                          VMX_STATUS(VMENTER_INVALID_VM_HOST_STATE)
#define STATUS_VM_ENTRY_WITH_INVALID_EXECUTIVE_VMCS_POINTER                     VMX_STATUS(VMENTRY_WITH_BAD_OSV_CONTROLLING_VMCS_ERROR)
#define STATUS_VM_ENTRY_WITH_NON_LAUNCHED_EXECUTIVE_VMCS                        VMX_STATUS(VMENTRY_WITH_NON_LAUNCHED_OSV_CONTROLLING_VMCS_ERROR)
#define STATUS_VM_ENTRY_WITH_EXECUTIVE_VMCS_POINTER_NOT_VMXON                   VMX_STATUS(VMENTRY_WITH_NON_ROOT_OSV_CONTROLLING_VMCS_ERROR)
#define STATUS_VM_ENTRY_WITH_INVALID_VM_EXEC_CONTROL_FIELDS_IN_EXECUTIVE_VMCS   VMX_STATUS(RETURN_FROM_SMM_WITH_BAD_VM_EXECUTION_CONTROLS_ERROR)
#define STATUS_VM_ENTRY_WITH_EVENTS_BLOCKED_BY_MOV_SS                           VMX_STATUS(VMENTRY_WITH_EVENTS_BLOCKED_BY_MOV_SS_ERROR)
#define STATUS_VMCALL_EXECUTED_IN_VMX_ROOT_OPERATION                            VMX_STATUS(VMCALL_IN_VMX_ROOT_ERROR)
#define STATUS_VMCALL_WITH_NON_CLEAR_VMCS                                       VMX_STATUS(VMCALL_WITH_NON_CLEAR_VMCS_ERROR)
#define STATUS_VMCALL_WITH_INVALID_VM_EXIT_CONTROL_FIELDS                       VMX_STATUS(VMCALL_WITH_BAD_VMEXIT_FIELDS_ERROR)
#define STATUS_VMCALL_WITH_INCORRECT_MSEG_REVISION_ID                           VMX_STATUS(VMCALL_WITH_INVALID_MSEG_REVISION_ERROR)
#define STATUS_VMCALL_WITH_INVALID_SMM_MONITOR_FEATURES                         VMX_STATUS(VMCALL_WITH_BAD_SMM_MONITOR_FEATURES_ERROR)
#define STATUS_VMPTRLD_WITH_INVALID_PHYSICAL_ADDRESS                            VMX_STATUS(VMPTRLD_INVALID_PHYSICAL_ADDRESS_ERROR)
#define STATUS_VMPTRLD_WITH_VMXON_POINTER                                       VMX_STATUS(VMPTRLD_WITH_CURRENT_CONTROLLING_PTR_ERROR)
#define STATUS_VMPTRLD_WITH_INCORRECT_VMCS_REVISION_IDENTIFIER                  VMX_STATUS(VMPTRLD_WITH_BAD_REVISION_ID_ERROR)
#define STATUS_VMCLEAR_WITH_INVALID_PHYSICAL_ADDRESS                            VMX_STATUS(VMCLEAR_WITH_INVALID_ADDR)
#define STATUS_VMCLEAR_WITH_VMXON_POINTER                                       VMX_STATUS(VMCLEAR_WITH_VMXON_VMCS_PTR)
#define STATUS_VMLAUNCH_WITH_NON_CLEAR_VMCS                                     VMX_STATUS(VMLAUNCH_WITH_NON_CLEAR_VMCS_ERROR)
#define STATUS_VMRESUME_WITH_NON_LAUNCHED_VMCS                                  VMX_STATUS(VMRESUME_WITH_NON_LAUNCHED_VMCS_ERROR)
#define STATUS_VMRESUME_AFTER_VMXOFF                                            VMX_STATUS(VMRESUME_VMCS_CORRUPTED)
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
#define STATUS_VMX_DIFFERENT_CONFIG_ACROSS_PROCESSORS                           VMX_STATUS(32)  //!< Mixed environment with different kind of processors with different capabilities
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


// FEATURE CONTROL MSR
#define IA32_FEATURE_CONTROL                            0x3a
#define IA32_FC_LOCK                     0x0001
#define IA32_FC_ENABLE_VMXON_OUTSMX 0x0004

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
    INTERRUPT_SOFTWARE_INTERRUPT   = 4,
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
typedef enum _VMX_VECTOR_EXCEPTION
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
} VMX_VECTOR_EXCEPTION;

#define INTR_INFO_VECTOR_MASK           0x1F

typedef union {
    struct {
        UINT32 lock:1;
        UINT32 enableVmxonInSmx:1;
        UINT32 enableVmxonOutsideSmx:1;
        UINT32 _reserved0:5;
        UINT32 senterEnables:8;
        UINT32 _reserved1:16;
        UINT32 _reserved2:32;
    } Bits;

    struct {
        UINT32 lower;
        UINT32 upper;
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
        UINT32 RevisionId : 31;
        UINT32 Rsvd31 : 1;
        UINT32 VmcsRegionSize : 13;
        UINT32 Rsvd45To47 : 3;
        UINT32 VmcsAddrWidth : 1;
        UINT32 DualMonitorTreatment : 1;
        UINT32 VmcsMemoryType : 4;
        UINT32 VmExitInstructionInformation : 1;
        UINT32 VmxControlDefaultClear : 1;
        UINT32 Rsvd56To63 : 8;
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
        UINT32 ExternalInterrupt : 1;
        UINT32 HostInterrupt : 1;
        UINT32 Init : 1;
        UINT32 NmiExiting : 1;
        UINT32 Sipi : 1;
        UINT32 VirtualNmi : 1;
        UINT32 VmxPreemptionTimer : 1;
        UINT32 PostedInterrupts : 1;
        UINT32 Rsvd8To31 : 24;
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
        UINT32 SoftwareInterrupt : 1;
        UINT32 TripleFault : 1;
        UINT32 InterruptWindow : 1;
        UINT32 TscOffsetting : 1;
        UINT32 TaskSwitch : 1;
        UINT32 Cpuid : 1;
        UINT32 GetSec : 1;
        UINT32 HltExiting : 1;
        UINT32 InvdExiting : 1;
        UINT32 InvlpgExiting : 1;
        UINT32 MwaitExiting : 1;
        UINT32 RdpmcExiting : 1;
        UINT32 RdtscExiting : 1;
        UINT32 Rsm : 1;
        UINT32 VmInstruction : 1; 
        UINT32 Cr3LoadExiting : 1;
        UINT32 Cr3StoreExiting : 1;
        UINT32 Cr3Mask : 1;
        UINT32 Cr3ReadShadow : 1; 
        UINT32 Cr8LoadExiting : 1;
        UINT32 Cr8StoreExiting : 1;
        UINT32 TprShadow : 1;
        UINT32 NmiWindowExiting : 1;
        UINT32 MovDrExiting : 1;
        UINT32 UnconditionalIoExiting : 1;
        UINT32 UseIoBitmap : 1; 
        UINT32 MsrProtection : 1;
        UINT32 MonitorTrapFlag : 1;
        UINT32 UseMsrBitmap : 1; 
        UINT32 MonitorExiting : 1;
        UINT32 PauseExiting : 1;
        UINT32 SecondaryControl : 1; 
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
        UINT32 VirtualApic : 1;
        UINT32 EnableEpt : 1;
        UINT32 DescriptorTableExiting : 1;
        UINT32 EnableRdtscp : 1;
        UINT32 VirtualizeX2Apic : 1;
        UINT32 EnableVpid : 1;
        UINT32 WbinvdExiting : 1;
        UINT32 UnrestrictedGuest : 1;
        UINT32 ApicRegisterVirtualization : 1;
        UINT32 VirtualInterruptDelivery : 1;
        UINT32 PauseLoopExiting : 1;
        UINT32 RdrandExiting : 1;
        UINT32 EnableInvpcid : 1;
        UINT32 EnableVmFunctions : 1;
        UINT32 VmcsShadowing : 1;
        UINT32 EnableEnclsExiting : 1;
        UINT32 RdseedExiting : 1;
        UINT32 EnablePml : 1;
        UINT32 EptVe : 1;
        UINT32 ConcealFromPT : 1;
        UINT32 EnableXsavesXrstors : 1;
        UINT32 Rsvd22 : 1;
        UINT32 ModeBasedExecuteControlEpt : 1;
        UINT32 Rsvd24 : 2;
        UINT32 UseTscScaling  : 1;
        UINT32 Rsvd26To31 : 6;
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
        UINT32 SaveCr0AndCr4 : 1;
        UINT32 SaveCr3 : 1;
        UINT32 SaveDebugControls : 1;
        UINT32 SaveSegmentRegisters : 1;
        UINT32 SaveEspEipEflags : 1;
        UINT32 SavePendingDebugExceptions : 1;
        UINT32 SaveInterruptibilityInformation : 1;
        UINT32 SaveActivityState : 1;
        UINT32 SaveWorkingVmcsPointer : 1;
        UINT32 Ia32eHost : 1;
        UINT32 LoadCr0AndCr4 : 1;
        UINT32 LoadCr3 : 1;
        UINT32 LoadIa32PerfGlobalControl : 1;
        UINT32 LoadSegmentRegisters : 1;
        UINT32 LoadEspEip : 1;
        UINT32 AcknowledgeInterrupt : 1;
        UINT32 SaveSysEnterMsrs : 1;
        UINT32 LoadSysEnterMsrs : 1;
        UINT32 SaveIa32Pat : 1;
        UINT32 LoadIa32Pat : 1;
        UINT32 SaveIa32Efer : 1;
        UINT32 LoadIa32Efer : 1;
        UINT32 SaveVmxPreemptionTimerValue : 1;
        UINT32 ClearIa32Bndcgfs : 1;
        UINT32 ConcealVmExitsFromPT : 1;
        UINT32 Rsvd25To310 : 7;
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
        UINT32 LoadCr0AndCr4 : 1;
        UINT32 LoadCr3 : 1; 
        UINT32 LoadDebugControls : 1;
        UINT32 LoadSegmentRegisters : 1;
        UINT32 LoadEspEipEflags : 1;
        UINT32 LoadPendingDebugExceptions : 1;
        UINT32 LoadInterruptibilityInformation : 1;
        UINT32 LoadActivityState : 1;
        UINT32 LoadWorkingVmcsPointer : 1;
        UINT32 Ia32eGuest : 1;
        UINT32 EntryToSmm : 1;
        UINT32 DeactivateDualMonitor : 1;
        UINT32 loadSysEnterMsrs : 1;
        UINT32 LoadIa32PerfGlobalControl  : 1;
        UINT32 LoadIa32Pat             : 1;
        UINT32 LoadIa32Efer            : 1;
        UINT32 LoadIa32Bndcgfs         : 1;
        UINT32 ConcealVmEntriesFromPT  : 1;
        UINT32 Rsvd18To31 : 14;
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

typedef union
{
    struct {
        UINT32 Reason : 16;
        UINT32 Rsvd16To27 : 12;
        UINT32 PendingMtf : 1;
        UINT32 FromVmxRootOperation : 1;
        UINT32 Rsvd30 : 1;
        UINT32 VmEntryFail : 1;
    } Bits;

    UINT32 AsUint32;
} VM_INFO_BASIC;

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
        UINT32 VmxPreemptionTimerRate : 5;
        UINT32 Ia32eToEFER : 1;
        UINT32 ActivityStateSupportHlt : 1;
        UINT32 ActivityStateSupportShutdown : 1;
        UINT32 ActivityStateSupportWaitForSipi : 1;
        UINT32 Rsvd9To14 : 6;
        UINT32 CanReadIa32SmbaseFromSmm : 1;
        UINT32 NumberOfCr3TargetValue : 9;
        UINT32 MaxNumberOfMsrInStoreList : 3;
        UINT32 VmxOffUnblockSmiSupport : 1;
        UINT32 CanWriteExitInfoFields : 1;
        UINT32 Rsvd30To31 : 2;
        UINT32 MsegRevisionId : 32;
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
        UINT32 x_only                                   : 1;
        UINT32 w_only                                   : 1;
        UINT32 w_and_x_only                             : 1;
        /* gaw support */
        UINT32 gaw_21_bit                               : 1;
        UINT32 gaw_30_bit                               : 1;
        UINT32 gaw_39_bit                               : 1;
        UINT32 gaw_48_bit                               : 1;
        UINT32 gaw_57_bit                               : 1;
        /* EMT support */
        UINT32 uc                                       : 1;
        UINT32 wc                                       : 1;
        UINT32 _reserved0                               : 2;
        UINT32 wt                                       : 1;
        UINT32 wp                                       : 1;
        UINT32 wb                                       : 1;
        UINT32 reserved1                                : 1;
        /* SP support */
        UINT32 sp_21_bit                                : 1;
        UINT32 sp_30_bit                                : 1;
        UINT32 sp_39_bit                                : 1;
        UINT32 sp_48_bit                                : 1;
        UINT32 supportInvept                            : 1;  // 20
        UINT32 supportDirtyAccessedBits                 : 1;  // 21
        UINT32 _reserved1                               : 2;  // 22 - 23
        UINT32 SupportInveptIndividualAddress           : 1;  // 24
        UINT32 supportInveptSingleContext               : 1;  // 25
        UINT32 supportInveptAllContext                  : 1;  // 26
        UINT32 _reserved2                               : 5;  // 27 - 31
        UINT32 supportInvvpid                           : 1;  // 32
        UINT32 _reserved3                               : 7;  // 33 - 39
        UINT32 supportInvvpidIndividualAddress          : 1;  // 40
        UINT32 supportInvvpidSingleContext              : 1;  // 41
        UINT32 supportInvvpidAllContext                 : 1;  // 42
        UINT32 supportInvvpidSingleContextRetainGlobals : 1;  // 43
        UINT32 _reserved4                               : 20; // 44 - 63
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
        UINT32 EptpSwitching  : 1;
        UINT32 Rsvd1To3 : 31;
        UINT32 Rsvd32To63 : 32;
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

typedef union _EXIT_GUEST_INTERRUPTIBILITY_STATE
{
    struct
    {
        UINT32 BlockingBySti : 1;
        UINT32 BlockingByMovSs : 1;
        UINT32 BlockingBySmi : 1;
        UINT32 BlockingByNmi : 1;
        UINT32 EnclaveInterruption : 1;
        UINT32 Rsvd5To31 : 27;
    } Bits;

    UINT32 AsUint32;
} EXIT_GUEST_INTERRUPTIBILITY_STATE;

typedef union _EXIT_QUALIFICATION_DEBUG_EXCEPTION
{
    struct
    {
        UINT32 Bp0 : 1;
        UINT32 Bp1 : 1;
        UINT32 Bp2 : 1;
        UINT32 Bp3 : 1;
        UINT32 Rsvd4To11 : 8;
        UINT32 EnableBreakpoint : 1;
        UINT32 Bd : 1;
        UINT32 Bs : 1;
        UINT32 Rsvd15To31 : 17;
#ifdef _WIN64
        UINT32 Rsvd32To63 : 32;
#endif
    } Bits;

    UINTN AsUintN;
} EXIT_QUALIFICATION_DEBUG_EXCEPTION;

typedef union _EXIT_QUALIFICATION_TASK_SWITCH
{
    struct
    {
        UINT32 Tss : 16;
        UINT32 Rsvd16To29 : 14;
        UINT32 TaskType : 2;
#ifdef _WIN64
        UINT32 Rsvd32To63 : 32;
#endif
    } Bits;

    UINTN AsUintN;
} EXIT_QUALIFICATION_TASK_SWITCH;

#define TASK_SWITCH_SOURCE_CALL             0
#define TASK_SWITCH_SOURCE_IRET             1
#define TASK_SWITCH_SOURCE_JMP              2
#define TASK_SWITCH_SOURCE_IDT_TASK_GATE    3

typedef union _EXIT_QUALIFICATION_CR
{
    struct
    {
        UINT32 Number : 4;
        UINT32 AccessType : 2;
        UINT32 LmswType : 1;
        UINT32 Rsvd7 : 1;
        UINT32 MoveGp : 4;
        UINT32 Rsvd12To15 : 4;
        UINT32 LmswSource : 16;
#ifdef _WIN64
        UINT32 Rsvd32To63 : 32;
#endif
    } CrAccess;

    UINTN AsUintN;
  
} EXIT_QUALIFICATION_CR, *PEXIT_QUALIFICATION_CR;

#define CR_ACCESS_TYPE_MOV_TO_CR    0
#define CR_ACCESS_TYPE_MOV_FROM_CR  1
#define CR_ACCESS_TYPE_CLTS         2
#define CR_ACCESS_TYPE_LMSW         3

#define CR_ACCESS_TYPE_LMSW_OPERAND_REGISTER    0
#define CR_ACCESS_TYPE_LMSW_OPERAND_MEMORY      1

typedef union _EXIT_QUALIFICATION_DR
{
    struct
    {
        UINT32 Number : 3;
        UINT32 Rsvd3 : 1;
        UINT32 Direction : 1;
        UINT32 Rsvd5To7 : 3;
        UINT32 MoveGp : 4;
        UINT32 Rsvd12To31 : 20;
#ifdef _WIN64
        UINT32 Rsvd32To63 : 32;
#endif
    } Bits;

    UINTN AsUintN;
} EXIT_QUALIFICATION_DR;

#define DR_ACCESS_TYPE_MOV_TO_DR    0
#define DR_ACCESS_TYPE_MOV_FROM_DR  1

typedef union _EXIT_QUALIFICATION_IO
{
   struct
    {
        UINT32 Size : 3;
        UINT32 Direction : 1;
        UINT32 StringInstruction : 1;
        UINT32 Rep : 1;
        UINT32 OperandEncoding : 1;
        UINT32 Rsvd7To15 : 9;
        UINT32 PortNum : 16;
#ifdef _WIN64
        UINT32 Rsvd32To63 : 32;
#endif
    } Bits;

    UINTN AsUintN;
} EXIT_QUALIFICATION_IO;

typedef struct _EXIT_QUALIFICATION_APIC
{
    union
    {
        UINTN raw;

        struct
        {
            UINT32 apicPageOffset    : 12;  // 0  - 11
            UINT32 accessType        : 4;   // 12 - 15
            UINT32 _reserved1        : 16;  // 16 - 31
#ifdef _WIN64
            UINT32 _reserved2        : 32; // 32 - 63
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
        UINTN raw;

        struct
        {
            UINT32 read              : 1;  // 0
            UINT32 write             : 1;  // 1
            UINT32 execute           : 1;  // 2
            UINT32 couldRead         : 1;  // 3
            UINT32 couldWrite        : 1;  // 4
            UINT32 couldExecute      : 1;  // 5
            UINT32 _reserved0        : 1;  // 6
            UINT32 guestLinearValid  : 1;  // 7
            UINT32 notPagingAccess   : 1;  // 8
            UINT32 _reserved1        : 3;  // 9 - 11
            UINT32 nmiUnblocking     : 1;  // 12
            UINT32 _reserved2        : 19; // 13 - 31
#ifdef _WIN64
            UINT32 _reserved3        : 32; // 32 - 63
#endif
        } f;
    } u;
} EXIT_QUALIFICATION_SLAT, *PEXIT_QUALIFICATION_SLAT;

typedef union _VMX_EXIT_INTERRUPT_INFO
{
    struct
    {
        UINT32 Vector : 8;
        UINT32 InterruptType : 3;
        UINT32 ErrorCodeValid : 1;
        UINT32 NmiUnblockingDueToIret : 1;
        UINT32 Rsvd13To30 : 18;
        UINT32 Valid : 1;
    } Bits;

    UINT32 AsUint32;
} VMX_EXIT_INTERRUPT_INFO;

typedef union _VM_ENTRY_CONTROL_INTERRUPT {
    
    struct
    {
        UINT32 Vector : 8;
        UINT32 InterruptType : 3;
        UINT32 DeliverErrorCode : 1;
        UINT32 Rsvd12To30 : 19;
        UINT32 Valid : 1;
    } Bits;

    UINT32 AsUint32;
} VM_ENTRY_CONTROL_INTERRUPT;

typedef union _VMX_IDT_VECTORING_INFORMATION {

    struct
    {
        UINT32 Vector : 8;
        UINT32 InterruptType : 3;
        UINT32 ErrorCodeValid : 1;
        UINT32 Rsvd12To30 : 19;
        UINT32 Valid : 1;

    } Bits;

    UINT32 AsUint32;
} VMX_IDT_VECTORING_INFORMATION;

#define INTERRUPT_TYPE_EXTERNAL             0
#define INTERRUPT_TYPE_NMI                  2
#define INTERRUPT_TYPE_HARDWARE_EXCEPTION   3
#define INTERRUPT_TYPE_SOFTWARE_EXCEPTION   6 //#BP - INT3  #OF - INTO


typedef struct _INSTRUCTION_INFORMATION_IO
{
    union
    {
        UINT32 raw;

        struct
        {
            UINT32 _reserved0  : 7;  // 0 - 6
            UINT32 addressSize : 3;  // 7 - 9
            UINT32 _reserved1  : 5;  // 10 - 14
            UINT32 segment     : 3;  // 15 - 17
            UINT32 _reserved2  : 14; // 18 - 31
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
            UINT32 scaling      : 2;  // 0  - 1
            UINT32 _reserved0   : 5;  // 2  - 6
            UINT32 addressSize  : 2;  // 7  - 9
            UINT32 _reserved1   : 5;  // 10 - 14
            UINT32 segment      : 3;  // 15 - 17
            UINT32 index        : 4;  // 18 - 21
            UINT32 indexInvalid : 1;  // 22
            UINT32 base         : 4;  // 23 - 26
            UINT32 baseInvalid  : 1;  // 27
            UINT32 reg2         : 1;  // 28 - 31
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
            UINT32 scaling      : 2; // 0 - 1
            UINT32 _reserved0   : 5; // 2 - 6
            UINT32 addressSize  : 3; // 7 - 9
            UINT32 _zero        : 1; // 10
            UINT32 operandSize  : 1; // 11
            UINT32 _reserved1   : 3; // 12 - 14
            UINT32 segment      : 3; // 15 - 17
            UINT32 index        : 4; // 18 - 21
            UINT32 indexInvalid : 1; // 22
            UINT32 base         : 4; // 23 - 26
            UINT32 baseInvalid  : 1; // 27
            UINT32 instruction  : 2; // 28 - 29
            UINT32 _reserved2   : 2; // 30 - 31
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
            UINT32 scaling      : 2; // 0 - 1
            UINT32 _reserved0   : 1; // 2
            UINT32 reg1         : 4; // 3 - 6
            UINT32 addressSize  : 3; // 7 - 9
            UINT32 memReg       : 1; // 10
            UINT32 _reserved1   : 4; // 11 - 14
            UINT32 segment      : 3; // 15 - 17
            UINT32 index        : 4; // 18 - 21
            UINT32 indexInvalid : 1; // 22
            UINT32 base         : 4; // 23 - 26
            UINT32 baseInvalid  : 1; // 27
            UINT32 instruction  : 2; // 28 - 29
            UINT32 _reserved2   : 2; // 30 - 31
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
            UINT32 _reserved0   : 3;  // 0  - 2
            UINT32 destination  : 4;  // 3  - 6
            UINT32 _reserved1   : 4;  // 7  - 10
            UINT32 operandSize  : 2;  // 11 - 12
            UINT32 _reserved2   : 19; // 13 - 31
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
VmxVerifySupport(
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
VmRead16(
    _In_ UINT32 field
);
UINT32
VmRead32(
    _In_ UINT32 field
);
UINT64
VmRead64(
    _In_ UINT32 field
);
UINTN
VmReadN(
    _In_ UINT32 field
);

/**
* Writes a field to the VMCS. Data can be 16, 32 or 64. In 32 bit systems 64 data writes are performed with to writes.
* This also holds true in 64 although in 64 the write could be done in one step.
*/
BOOLEAN
VmWrite16(
    _In_ UINT32 field,
    _In_ UINT16 data
);
BOOLEAN
VmWrite32(
    _In_ UINT32 field,
    _In_ UINT32 data
);
BOOLEAN
VmWrite64(
    _In_ UINT32 field,
    _In_ UINT64 data
);
BOOLEAN
VmWriteN(
    _In_ UINT32   field,
    _In_ UINTN data
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
    _In_ UINTN service,
    _In_ UINTN data
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
    _In_ UINTN service,
    _In_ UINTN data
);

/**
* VMX Instructions: (we have only intrinsics for 64 bit systems)
* Return values:
*  - 0 = The operation succeeded.
*  - 1 = The operation failed with extended status available in the VM - instruction error field of the current VMCS.
*  - 2 = The operation failed without Status available.
*/
#ifndef _WIN64

UINT8 __stdcall AsmVmxOn     (PUINT64 vmxOnAddress);
#define AsmVmxOff __vmx_off
UINT8 __stdcall AsmVmxClear(PUINT64 vmcsAddress);
UINT8 __stdcall AsmVmxPtrLoad(PUINT64 vmcsAddress);
UINT8 __stdcall AsmVmxPtrStore(PUINT64 vmcsAddress);
UINT8 __stdcall AsmVmxRead (UINT32 field, PUINTN data);
UINT8 __stdcall AsmVmxWrite(UINT32 field, UINTN  data);
UINT8 __stdcall VmxInvEpt (UINTN type, PVOID eptPointer);
UINT8 __stdcall AsmVmxInvVpid(IVVPID_TYPE type, PVPID_CTX vpidContext );

#else

#define AsmVmxOn __vmx_on
#define AsmVmxOff __vmx_off
#define AsmVmxClear __vmx_vmclear
#define AsmVmxPtrLoad __vmx_vmptrld
#define AsmVmxPtrStore __vmx_vmptrst
#define AsmVmxRead __vmx_vmread
#define AsmVmxWrite __vmx_vmwrite
UINT8 AsmVmxInvVpid(_In_ IVVPID_TYPE type, _In_ PVPID_CTX vpidContext);
UINT8 VmxInvEpt(_In_ UINTN type, _In_ PVOID eptPointer);
#endif

VOID
VmInjectUdException(
    VOID
    );

VOID
VmInjectGpException(
    _In_ UINT32 ErrorCode
    );

VOID
VmInjectHardwareException(
    _In_ VMX_EXIT_INTERRUPT_INFO InterruptInfo
    );

VOID
VmInjectInterruptOrException(
    _In_ VMX_EXIT_INTERRUPT_INFO InterruptInfo
    );

BOOLEAN
IsRdtscpSupported(
    VOID
    );

BOOLEAN
IsInvpcidSupported(
    VOID
    );

BOOLEAN
IsXStateSupported(
    VOID
    );

BOOLEAN
IsXStateEnabled(
    VOID
    );

#endif
