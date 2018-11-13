.686P
.XMM
.MODEL flat, C

extern HvmpEnterRoot@16:PROC
extern HvmpStartFailure@8:PROC
extern HvmpExitHandler@8:PROC
extern HvmpFailure@8:PROC
extern KeBugCheck@4:PROC

.CONST

HVM_CALL_MAGIC             EQU  0CAFEBABEh

; Offset definitions for GP.
EAX_SAVE EQU 00000H
ECX_SAVE EQU 00004H
EDX_SAVE EQU 00008H
EBX_SAVE EQU 0000CH
ESP_SAVE EQU 00010H
EBP_SAVE EQU 00014H
ESI_SAVE EQU 00018H
EDI_SAVE EQU 0001CH

; Offset definitions for XMM.
XMM0_SAVE  EQU 00020H
XMM1_SAVE  EQU 00030H
XMM2_SAVE  EQU 00040H
XMM3_SAVE  EQU 00050H
XMM4_SAVE  EQU 00060H
XMM5_SAVE  EQU 00070H
XMM6_SAVE  EQU 00080H
XMM7_SAVE  EQU 00090H

; Offset definition for MXCSR.
MXCSR_SAVE  EQU 000A0H

; Saves all general purpose registers to the stack
CPU_SAVE_REGS MACRO
    ; preserve rbx
    push ebx

    ; load guest registers save area
    lea ebx, [esp+3Ch+4h]
    mov ebx, [ebx]

    ; save eax and then ebx
    mov EAX_SAVE[ebx], eax
    pop eax
    mov EBX_SAVE[ebx], eax

    ; save all GP except esp, eip & eflags
    mov ECX_SAVE[ebx], ecx
    mov EDX_SAVE[ebx], edx
    mov EBP_SAVE[ebx], ebp
    mov ESI_SAVE[ebx], esi
    mov EDI_SAVE[ebx], edi

    ; save all XMM by last
    movdqa XMM0_SAVE[ebx], xmm0
    movdqa XMM1_SAVE[ebx], xmm1
    movdqa XMM2_SAVE[ebx], xmm2
    movdqa XMM3_SAVE[ebx], xmm3
    movdqa XMM4_SAVE[ebx], xmm4
    movdqa XMM5_SAVE[ebx], xmm5
    movdqa XMM6_SAVE[ebx], xmm6
    movdqa XMM7_SAVE[ebx], xmm7

    ; save XMM floating state
    stmxcsr MXCSR_SAVE[ebx]
ENDM

; Loads all general purpose registers from the stack
CPU_RESTORE_REGS MACRO
    ; restore all XMM first
    movdqa  xmm0, XMM0_SAVE[ebx]
    movdqa  xmm1, XMM1_SAVE[ebx]
    movdqa  xmm2, XMM2_SAVE[ebx]
    movdqa  xmm3, XMM3_SAVE[ebx]
    movdqa  xmm4, XMM4_SAVE[ebx]
    movdqa  xmm5, XMM5_SAVE[ebx]
    movdqa  xmm6, XMM6_SAVE[ebx]
    movdqa  xmm7, XMM7_SAVE[ebx]

	; restore MXCSR
    ldmxcsr MXCSR_SAVE[ebx]

    ; restore all GP except rcx
    mov eax, EAX_SAVE[ebx]
    mov ecx, ECX_SAVE[ebx]
    mov edx, EDX_SAVE[ebx]
    mov ebp, EBP_SAVE[ebx]
    mov esi, ESI_SAVE[ebx]
    mov edi, EDI_SAVE[ebx]
    
    ; by last restore rbx
    mov ebx, EBX_SAVE[ebx]
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
; NTSTATUS __stdcall AsmHvmLaunch(_In_ PHVM_VCPU Vcpu);
;
AsmHvmLaunch@4 PROC
    push ebp
    mov ebp, esp

    ;
    ; Push Vcpu to make it available in case of failure
    ;
    push dword ptr [ebp + 8]

    ;
    ; Call HvmpEnterRoot injecting rip and rsp for successful vmlaunch
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
    call HvmpEnterRoot@16

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
AsmHvmLaunch@4 ENDP


;
; VOID __stdcall AsmHvmpStop(_In_ UINTN iret, _In_ UINTN Registers);
;
AsmHvmpStop@8 PROC
    mov esp, [esp + 4]
    mov ebx, [esp + 8]
    CPU_RESTORE_REGS
    iretd
AsmHvmpStop@8 ENDP

;
; VOID AsmHvmpExitHandler(VOID);
;
AsmHvmpExitHandler@0 PROC
    ;
    ; Save all registers
    ;
    CPU_SAVE_REGS

    ;
    ; Second parameter = registers
    ;
    mov eax, ebx
    push ebx

    ;
    ; First parameter = hvm_vcpu
    ;
    or   eax, 0FFCh   ; set pointer to top of page where address of hvm_cpu is stored
    mov  eax, [eax]   ; dereference to get the pointer
    push eax
    call HvmpExitHandler@8

    ;
    ; Restore (possibly modified) registers
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
    CALLF_VM_ENTRY_FAILURE_HANDLER HvmpFailure@8

    push eax
    call KeBugCheck@4
AsmHvmpExitHandler@0 ENDP


;
; NTSTATUS __stdcall AsmHvmHyperCall(_In_ UINTN service, _In_ UINTN data);
;
AsmHvmHyperCall@8 PROC
    mov ecx, [esp + 4]
    mov edx, [esp + 8]
    mov eax, HVM_CALL_MAGIC

    vmcall

    nop
    ret 8
AsmHvmHyperCall@8 ENDP


END
