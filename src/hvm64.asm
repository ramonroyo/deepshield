
extern HvmpStart:PROC
extern HvmpStartFailure:PROC
extern HvmpExitHandler:PROC
extern HvmpFailure:PROC
extern KeBugCheck:PROC

.CONST

HVM_ADDITIONAL_REGS_SIZE   EQU  (2 * 8)
HVM_CALL_MAGIC             EQU  0CAFEBABEh

; Saves all general purpose registers to the stack
PUSHALL MACRO
    sub  rsp, HVM_ADDITIONAL_REGS_SIZE
    push rax
    push rcx
    push rdx
    push rbx
    push -1      ; dummy for rsp
    push rbp
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
ENDM

; Loads all general purpose registers from the stack
POPALL MACRO
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rbp
    add rsp, 8    ; dummy for rsp
    pop rbx
    pop rdx
    pop rcx
    pop rax
    add rsp, HVM_ADDITIONAL_REGS_SIZE
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
; NTSTATUS HvmpStartAsm(_In_ PHVM_CORE core);
;
HvmpStartAsm PROC
    ;
    ; Save used regs
    ;
    push r9
    push r8
    push rbx
    push rdx
    push rcx

    ;
    ; Call HvmpStart injecting rip and rsp for successful vmlaunch
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
    CALLF HvmpStart

    ;
    ; Check if configuration was successful. If configuration failed, function performed switch off of vmx mode, since we need only to report error
    ;
    cmp rax, 0
    jnz finish

    vmlaunch       ; we make the instruction return to the same point in case of success or failure to ease coding
vmlaunchReturn:
    CHECK_VM_ENTRY

    ;
    ; Success
    ;
    xor rax, rax
    jmp finish

    ;
    ; Failure
    ;
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
; VOID HvmpStopAsm( _In_ UINT_PTR stack);
;
HvmpStopAsm PROC
    mov rsp, rcx
    POPALL
    iretq
HvmpStopAsm ENDP



;
; VOID HvmpExitHandlerAsm(VOID);
;
HvmpExitHandlerAsm PROC
    ;
    ; Save all registers
    ;
    PUSHALL

    ;
    ; Second parameter = registers
    ;
    mov rdx, rsp

    ;
    ; First parameter = hvm_core
    ;
    mov rcx, rsp
    or  rcx, 0FF8h    ; set pointer to top of page where address of hvm_core is stored
    mov rcx, [rcx]    ; dereference to get the pointer

    CALLF HvmpExitHandler

    ;
    ; Restore (possibly modified) registers
    ;
    POPALL

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
