.686p
.MODEL flat, C

.CONST

VMX_OK                      EQU     0
VMX_ERROR_WITH_STATUS       EQU     1
VMX_ERROR_WITHOUT_STATUS    EQU     2

.CODE

VmxOn@4 PROC
    mov eax, [esp + 4]

    vmxon qword ptr [eax]

    jz failValid
    jc failInvalid
    mov al, 0
    ret 4

failValid:
    mov al, 1
    ret 4

failInvalid:
    mov al, 2
    ret 4
VmxOn@4 ENDP


VmxVmClear@4 PROC
    mov eax, [esp + 4]

    vmclear qword ptr [eax]

    jz failValid
    jc failInvalid
    mov al, 0
    ret 4

failValid:
    mov al, 1
    ret 4

failInvalid:
    mov al, 2
    ret 4
VmxVmClear@4 ENDP


VmxVmPtrLd@4 PROC
    mov eax, [esp + 4]

    vmptrld qword ptr [eax]

    jz failValid
    jc failInvalid
    mov al, 0
    ret 4

failValid:
    mov al, 1
    ret 4

failInvalid:
    mov al, 2
    ret 4
VmxVmPtrLd@4 ENDP


VmxVmPtrSt@4 PROC
    mov eax, [esp + 4]

    ;vmptrst
    BYTE 0Fh, 0C7h, 38h

    jz failValid
    jc failInvalid
    mov al, 0
    ret 4

failValid:
    mov al, 1
    ret 4

failInvalid:
    mov al, 2
    ret 4
VmxVmPtrSt@4 ENDP


VmxVmRead@8 PROC
    mov eax, [esp + 4]
    mov edx, [esp + 8]

    vmread dword ptr [edx], eax

    jz failValid
    jc failInvalid
    mov al, 0
    ret 8

failValid:
    mov al, 1
    ret 8

failInvalid:
    mov al, 2
    ret 8
VmxVmRead@8 ENDP


VmxVmWrite@8 PROC
    mov eax, [esp + 4]
    mov edx, [esp + 8]

    vmwrite eax, edx

    jz failValid
    jc failInvalid
    mov al, 0
    ret 8

failValid:
    mov al, 1
    ret 8

failInvalid:
    mov al, 2
    ret 8
VmxVmWrite@8 ENDP


VmxInvEptImpl@8 PROC
    mov ecx, [esp + 4]
    mov edx, [esp + 8]

    ;invept ecx, oword ptr [edx]
    BYTE 66h, 0Fh, 38h, 80h, 0Ah

    jz failValid
    jc failInvalid

    ; return VMX_OK
    xor eax, eax
    ret 8

failValid:
    mov eax, VMX_ERROR_WITH_STATUS
    ret 8

failInvalid:
    mov eax, VMX_ERROR_WITHOUT_STATUS
    ret 8
VmxInvEptImpl@8 ENDP

VmxInvVpid@8 PROC
    mov ecx, [esp + 4]
    mov edx, [esp + 8]

    ;invvpid ecx, oword prt [edx]
    BYTE 66h, 0Fh, 38h, 81h, 0Ah

    jz failValid
    jc failInvalid

    ; return VMX_OK
    xor eax, eax
    ret 8

failValid:
    mov eax, VMX_ERROR_WITH_STATUS
    ret 8

failInvalid:
    mov eax, VMX_ERROR_WITHOUT_STATUS
    ret 8
VmxInvVpid@8 ENDP

;
; NTSTATUS __stdcall VmxCall(_In_ UINT_PTR service, _In_ UINT_PTR data);
;
VmxCall@8 PROC
    mov ecx, [esp + 4]
    mov edx, [esp + 8]

    vmcall

    nop
    ret 8
VmxCall@8 ENDP

;
; NTSTATUS VmxFunc(_In_ UINT_PTR service, _In_ UINT_PTR data);
;
VmxFunc@8 PROC
    mov eax, [esp + 4]
    mov ecx, [esp + 8]

    ;vmfunc
    BYTE 0Fh, 01h, 0D4h

    nop
    ret 8
VmxFunc@8 ENDP

END
