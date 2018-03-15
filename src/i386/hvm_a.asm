.686p
.MODEL flat, C

extern HvmpStart@16:PROC
extern HvmpStartFailure@8:PROC
extern HvmpExitHandler@8:PROC
extern HvmpFailure@8:PROC
extern KeBugCheck@4:PROC

.CONST

HVM_ADDITIONAL_REGS_SIZE   EQU  (2 * 4)
HvM_CALL_MAGIC             EQU  0CAFEBABEh

; Saves all general purpose registers to the stack
PUSHALL MACRO
    sub esp, HVM_ADDITIONAL_REGS_SIZE
    pushad
ENDM

; Loads all general purpose registers from the stack
POPALL MACRO
    popad
    add esp, HVM_ADDITIONAL_REGS_SIZE
ENDM


CHECK_VM_ENTRY MACRO
    nop
    jz failValid
    jc failInvalid
ENDM


CALLF_VM_ENTRY_FAILURE_HANDLER MACRO X
failValid:
    push 1
    jmp fail
failInvalid:
    push 0
fail:
    push dword ptr [esp + 4]
    call X
ENDM

.CODE

;
; NTSTATUS __stdcall HvmpStartAsm(_In_ PHVM_CORE core);
;
HvmpStartAsm@4 PROC
    push ebp
    mov ebp, esp

    ;
    ; Push core to make it available in case of failure
    ;
    push dword ptr [ebp + 8]

    ;
    ; Call HvmpStart injecting rip and rsp for successful vmlaunch
    ;
        ;Fourth Parameter
    push dword ptr [ebp + 8]

        ;Third Parameter
    pushfd

        ;Second Parameter
    push ebp

        ;First Parameter
    push vmlaunchReturn

        ;Call
    call HvmpStart@16

    ;
    ; Check if configuration was successful. If configuration failed, function performed switch off of vmx mode, since we need only to report error
    ;
    cmp eax, 0
    jnz finish

    vmlaunch       ; we make the instruction return to the same point in case of success or failure to ease coding

vmlaunchReturn:
    CHECK_VM_ENTRY

    ;
    ; Success
    ;
    xor eax, eax
    jmp finish

    CALLF_VM_ENTRY_FAILURE_HANDLER HvmpStartFailure@8

    add esp, 4

finish:
    pop ebp
    ret 4
HvmpStartAsm@4 ENDP


;
; VOID __stdcall HvmpStopAsm( _In_ UINT_PTR stack);
;
HvmpStopAsm@4 PROC
    mov esp, [esp + 4]
    POPALL
    pop esp
    iretd
HvmpStopAsm@4 ENDP


;
; VOID HvmpExitHandlerAsm(VOID);
;
HvmpExitHandlerAsm@0 PROC
    ;
    ; Save all registers
    ;
    PUSHALL

    ;
    ; Second parameter = registers
    ;
    mov eax, esp
    push esp

    ;
    ; First parameter = hvm_core
    ;
    or   eax, 0FFCh   ; set pointer to top of page where address of hvm_core is stored
    mov  eax, [eax]   ; dereference to get the pointer
    push eax
    call HvmpExitHandler@8

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
    CALLF_VM_ENTRY_FAILURE_HANDLER HvmpFailure@8

    push eax
    call KeBugCheck@4
HvmpExitHandlerAsm@0 ENDP


;
; NTSTATUS __stdcall HvmInternalCallAsm(_In_ UINT_PTR service, _In_ UINT_PTR data);
;
HvmInternalCallAsm@8 PROC
    mov ecx, [esp + 4]
    mov edx, [esp + 8]
    mov eax, HVM_CALL_MAGIC

    vmcall

    nop
    ret 8
HvmInternalCallAsm@8 ENDP


END
