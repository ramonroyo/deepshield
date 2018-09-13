
extern HvmpEnterRoot:PROC
extern HvmpStartFailure:PROC
extern HvmpExitHandler:PROC
extern HvmpFailure:PROC
extern KeBugCheck:PROC

.CONST

HVM_CALL_MAGIC       EQU  0CAFEBABEh

; Offset definitions for GP.
RAX_SAVE EQU 00000H
RCX_SAVE EQU 00008H
RDX_SAVE EQU 00010H
RBX_SAVE EQU 00018H
RSP_SAVE EQU 00020H
RBP_SAVE EQU 00028H
RSI_SAVE EQU 00030H
RDI_SAVE EQU 00038H
R8_SAVE  EQU 00040H
R9_SAVE  EQU 00048H
R10_SAVE EQU 00050H
R11_SAVE EQU 00058H
R12_SAVE EQU 00060H
R13_SAVE EQU 00068H
R14_SAVE EQU 00070H
R15_SAVE EQU 00078H

; Offset definition for XMM.
XMM0_SAVE  EQU 00080H
XMM1_SAVE  EQU 00090H
XMM2_SAVE  EQU 000A0H
XMM3_SAVE  EQU 000B0H
XMM4_SAVE  EQU 000C0H
XMM5_SAVE  EQU 000D0H
XMM6_SAVE  EQU 000E0H
XMM7_SAVE  EQU 000F0H
XMM8_SAVE  EQU 00100H
XMM9_SAVE  EQU 00110H
XMM10_SAVE EQU 00120H
XMM11_SAVE EQU 00130H
XMM12_SAVE EQU 00140H
XMM13_SAVE EQU 00150H
XMM14_SAVE EQU 00160H
XMM15_SAVE EQU 00170H

; Saves CPU registers to the guest area
CPU_SAVE_REGS MACRO
    ; preserve rbx
    push rbx

    ; load guest registers save area
    lea rbx, [rsp+38h+8h]
    mov rbx, [rbx]

    ; save rax and then rbx
    mov RAX_SAVE[rbx], rax
    pop rax
    mov RBX_SAVE[rbx], rax

    ; save all GP except rsp, rip & rflags
    mov RCX_SAVE[rbx], rcx
    mov RDX_SAVE[rbx], rdx
    mov RBP_SAVE[rbx], rbp
    mov RSI_SAVE[rbx], rsi
    mov RDI_SAVE[rbx], rdi
    mov R8_SAVE[rbx], r8
    mov R9_SAVE[rbx], r9
    mov R10_SAVE[rbx], r10
    mov R11_SAVE[rbx], r11
    mov R12_SAVE[rbx], r12
    mov R13_SAVE[rbx], r13
    mov R14_SAVE[rbx], r14
    mov R15_SAVE[rbx], r15

    ; save all XMM by last
    movdqa XMM0_SAVE[rbx], xmm0
    movdqa XMM1_SAVE[rbx], xmm1
    movdqa XMM2_SAVE[rbx], xmm2
    movdqa XMM3_SAVE[rbx], xmm3
    movdqa XMM4_SAVE[rbx], xmm4
    movdqa XMM5_SAVE[rbx], xmm5
    movdqa XMM6_SAVE[rbx], xmm6
    movdqa XMM7_SAVE[rbx], xmm7
    movdqa XMM8_SAVE[rbx], xmm8
    movdqa XMM9_SAVE[rbx], xmm9
    movdqa XMM10_SAVE[rbx], xmm10
    movdqa XMM11_SAVE[rbx], xmm11
    movdqa XMM12_SAVE[rbx], xmm12
    movdqa XMM13_SAVE[rbx], xmm13
    movdqa XMM14_SAVE[rbx], xmm14
    movdqa XMM15_SAVE[rbx], xmm15
ENDM


; Restores CPU registers from guest area
CPU_RESTORE_REGS MACRO

    ; restore all XMM first
    movdqa  xmm0, XMM0_SAVE[rbx]
    movdqa  xmm1, XMM1_SAVE[rbx]
    movdqa  xmm2, XMM2_SAVE[rbx]
    movdqa  xmm3, XMM3_SAVE[rbx]
    movdqa  xmm4, XMM4_SAVE[rbx]
    movdqa  xmm5, XMM5_SAVE[rbx]
    movdqa  xmm6, XMM6_SAVE[rbx]
    movdqa  xmm7, XMM7_SAVE[rbx]
    movdqa  xmm8, XMM8_SAVE[rbx]
    movdqa  xmm9, XMM9_SAVE[rbx]
    movdqa  xmm10, XMM10_SAVE[rbx] 
    movdqa  xmm11, XMM11_SAVE[rbx] 
    movdqa  xmm12, XMM12_SAVE[rbx] 
    movdqa  xmm13, XMM13_SAVE[rbx] 
    movdqa  xmm14, XMM14_SAVE[rbx] 
    movdqa  xmm15, XMM15_SAVE[rbx]
    
    ; restore all GP except rcx
    mov rax, RAX_SAVE[rbx]
    mov rcx, RCX_SAVE[rbx]
    mov rdx, RDX_SAVE[rbx]
    mov rbp, RBP_SAVE[rbx]
    mov rsi, RSI_SAVE[rbx]
    mov rdi, RDI_SAVE[rbx]
    mov r8,  R8_SAVE[rbx]
    mov r9,  R9_SAVE[rbx]
    mov r10, R10_SAVE[rbx]
    mov r11, R11_SAVE[rbx]
    mov r12, R12_SAVE[rbx]
    mov r13, R13_SAVE[rbx]
    mov r14, R14_SAVE[rbx]
    mov r15, R15_SAVE[rbx]

    ; by last restore rbx
    mov rbx, RBX_SAVE[rbx]
ENDM


CALLF MACRO X
    mov rbx, rsp
    and rsp, 0FFFFFFFFFFFFFFF0h
    sub rsp, 20h
    call X
    mov rsp, rbx
ENDM


CHECK_VM_ENTRY MACRO
    nop
    jz failValid
    jc failInvalid
ENDM


CALLF_VM_ENTRY_FAILURE_HANDLER MACRO X
failValid:
    mov rdx, 1
    jmp fail

failInvalid:
    xor rdx, rdx

fail:
    mov rcx, [rsp]
    CALLF X
ENDM


.CODE

;
; NTSTATUS HvmpStartAsm(_In_ PHVM_VCPU Vcpu);
;
HvmpStartAsm PROC
    ;
    ; Save used Registers
    ;
    push r9
    push r8
    push rbx
    push rdx
    push rcx

    ;
    ; Call HvmpEnterRoot injecting rip and rsp for successful vmlaunch
    ;
    ;Fourth Parameter
    mov r9, rcx
    
    ;Third parameter
    pushfq
    pop r8

    ;Second parameter
    mov rdx, rsp

    ;First Parameter
    mov rcx, vmlaunchReturn

    ;Call
    CALLF HvmpEnterRoot

    ;
    ; Check if configuration was successful. If configuration failed, function
    ; performed switch off of vmx mode, since we need only to report error.
    ;
    cmp rax, 0
    jnz finish

    ; Make the instruction return to the same point in case of success or
    ; failure to ease coding.
    vmlaunch

vmlaunchReturn:
    CHECK_VM_ENTRY

    ; Success
    xor rax, rax
    jmp finish

    ; Failure
    CALLF_VM_ENTRY_FAILURE_HANDLER HvmpStartFailure

finish:
    pop rcx
    pop rdx
    pop rbx
    pop r8
    pop r9
    ret
HvmpStartAsm ENDP


;
; VOID HvmpStopAsm( _In_ UINT_PTR iret, _In_ UINT_PTR Registers);
;
HvmpStopAsm PROC
    mov rsp, rcx
    mov rbx, rdx
    CPU_RESTORE_REGS
    iretq
HvmpStopAsm ENDP


;
; VOID HvmpExitHandlerAsm(VOID);
;
HvmpExitHandlerAsm PROC
    ;
    ; Save all registers
    ;
    CPU_SAVE_REGS

    ; Second parameter = Registers
    mov rdx, rbx

    ;
    ; First parameter = hvm_cpu
    ;
    mov rcx, rsp
    or  rcx, 0FF8h    ; set pointer to top of page where address of hvm_cpu is stored
    mov rcx, [rcx]    ; dereference to get the pointer

    CALLF HvmpExitHandler

    ; load guest registers save area
    lea rbx, [rsp+38h]
    mov rbx, [rbx]

    ;
    ; Restore guest registers
    ;
    CPU_RESTORE_REGS

    ;
    ; Resume
    ;
    vmresume

    ;
    ; Failure on resume -> BSOD
    ;
    CHECK_VM_ENTRY
    CALLF_VM_ENTRY_FAILURE_HANDLER HvmpFailure

    mov rcx, rax
    CALLF KeBugCheck
HvmpExitHandlerAsm ENDP


;
; NTSTATUS HvmInternalCallAsm(_In_ UINT_PTR service, _In_ UINT_PTR data);
;
HvmInternalCallAsm PROC
    mov rax, HVM_CALL_MAGIC
    vmcall
    nop
    ret
HvmInternalCallAsm ENDP

END
