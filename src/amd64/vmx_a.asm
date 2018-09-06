.CODE

VmxInvEptImpl PROC

    ;invept ecx, [edx]
    BYTE 66h, 0Fh, 38h, 80h, 0Ah

    jz failValid
    jc failInvalid
    mov al, 0
    ret

failValid:
    mov al, 1
    ret

failInvalid:
    mov al, 2
    ret
VmxInvEptImpl ENDP

__invvpid PROC
    invvpid rcx, OWORD PTR [rdx]
    ret
__invvpid ENDP

;
; NTSTATUS VmxCall(_In_ UINT_PTR service, _In_ UINT_PTR data);
;
VmxCall PROC
    vmcall

    nop
    ret
VmxCall ENDP

;
; NTSTATUS VmxVmFunc(_In_ UINT_PTR service, _In_ UINT_PTR data);
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
