.686p
.MODEL flat, C

.CODE

__readcs@0 PROC
    xor eax, eax
    mov ax, cs
    ret
__readcs@0 ENDP


__readss@0 PROC
    xor eax, eax
    mov ax, ss
    ret
__readss@0 ENDP


__readds@0 PROC
    xor eax, eax
    mov ax, ds
    ret
__readds@0 ENDP


__reades@0 PROC
    xor eax, eax
    mov ax, es
    ret
__reades@0 ENDP


__readfs@0 PROC
    xor eax, eax
    mov ax, fs
    ret
__readfs@0 ENDP


__readgs@0 PROC
    xor eax, eax
    mov ax, gs
    ret
__readgs@0 ENDP


__str@0 PROC
    xor eax, eax
    str ax
    ret
__str@0 ENDP


__sldt@0 PROC
    xor eax, eax
    sldt ax
    ret
__sldt@0 ENDP


__writess@4 PROC
    mov eax, [esp + 4]
    mov ss, ax
    ret 4
__writess@4 ENDP


__writeds@4 PROC
    mov eax, [esp + 4]
    mov ds, ax
    ret 4
__writeds@4 ENDP


__writees@4 PROC
    mov eax, [esp + 4]
    mov es, ax
    ret 4
__writees@4 ENDP


__writefs@4 PROC
    mov eax, [esp + 4]
    mov fs, ax
    ret 4
__writefs@4 ENDP


__writegs@4 PROC
    mov eax, [esp + 4]
    mov gs, ax
    ret 4
__writegs@4 ENDP


__ltr@4 PROC
    mov eax, [esp + 4]
    ltr ax
    ret 4
__ltr@4 ENDP


DescriptorLimit@4 PROC
    mov ecx, [esp + 4]
    lsl eax, ecx
    ret 4
DescriptorLimit@4 ENDP


__readsp@0 PROC
    mov eax, esp
    ret
__readsp@0 ENDP


__readflags@0 PROC
    pushfd
    pop eax
    ret
__readflags@0 ENDP


__writecr2@4 PROC
    mov eax, [esp + 4]
    mov cr2, eax
    ret 4
__writecr2@4 ENDP


__cli@0 PROC
    cli
    ret
__cli@0 ENDP


__sti@0 PROC
    sti
    ret
__sti@0 ENDP


__pause@0 PROC
    pause
    ret
__pause@0 ENDP


__sgdt@4 PROC
    mov eax, [esp + 4]
    sgdt [eax]
    ret 4
__sgdt@4 ENDP

__lgdt@4 PROC
    mov eax, [esp + 4]
    lgdt fword ptr [eax]
    ret 4
__lgdt@4 ENDP

END
