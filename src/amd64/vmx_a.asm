;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; constants
;
.CONST

VMX_OK                      EQU     0
VMX_ERROR_WITH_STATUS       EQU     1
VMX_ERROR_WITHOUT_STATUS    EQU     2

.CODE

AsmVmxInvEpt PROC
    ;invept rcx, oword ptr [rdx]
    BYTE 66h, 0Fh, 38h, 80h, 0Ah

    jz failValid
    jc failInvalid

    ; return VMX_OK
    xor rax, rax
    ret

failValid:
    mov rax, VMX_ERROR_WITH_STATUS
    ret

failInvalid:
    mov rax, VMX_ERROR_WITHOUT_STATUS
    ret
AsmVmxInvEpt ENDP

AsmVmxInvVpid PROC
    ;invvpid rcx, oword ptr [rdx]
    BYTE 66h, 0Fh, 38h, 81h, 0Ah

    jz failValid
    jc failInvalid

    ; return VMX_OK
    xor rax, rax
    ret

failValid:
    mov rax, VMX_ERROR_WITH_STATUS
    ret

failInvalid:
    mov rax, VMX_ERROR_WITHOUT_STATUS
    ret
AsmVmxInvVpid ENDP

;
; NTSTATUS VmxCall(_In_ UINTN service, _In_ UINTN data);
;
VmxCall PROC
    vmcall

    nop
    ret
VmxCall ENDP

;
; NTSTATUS VmxVmFunc(_In_ UINTN service, _In_ UINTN data);
;
VmxFunc PROC
    mov rax, rcx
    mov rcx, rdx

    ;vmfunc
    BYTE 0Fh, 01h, 0D4h

    nop
    ret
VmxFunc ENDP

END
