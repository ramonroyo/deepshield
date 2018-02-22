.CODE

__readcs PROC
    xor rax, rax
    mov ax, cs
    ret
__readcs ENDP


__readss PROC
    xor rax, rax
    mov ax, ss
    ret
__readss ENDP


__readds PROC
    xor rax, rax
    mov	ax, ds
    ret
__readds ENDP


__reades PROC
    xor rax, rax
    mov	ax, es
    ret
__reades ENDP


__readfs PROC
    xor rax, rax
    mov	ax, fs
    ret
__readfs ENDP


__readgs PROC
    xor rax, rax
    mov	ax, gs
    ret
__readgs ENDP


__str PROC
    xor rax, rax
    str	ax
    ret
__str ENDP


__sldt PROC
    xor rax, rax
    sldt ax
    ret
__sldt ENDP


__writess PROC
    mov ss, cx
    ret
__writess ENDP


__writeds PROC
    mov ds, cx
    ret
__writeds ENDP


__writees PROC
    mov es, cx
    ret
__writees ENDP


__writefs PROC
    mov fs, cx
    ret
__writefs ENDP


__writegs PROC
    mov gs, cx
    ret
__writegs ENDP


__ltr PROC
    ltr cx
    ret
__ltr ENDP


DescriptorLimit PROC
    lsl rax, rcx
    ret
DescriptorLimit ENDP


__readsp PROC
    mov rax, rsp
    ret
__readsp ENDP


__readflags PROC
    pushfq
    pop rax
    ret
__readflags ENDP


__writecr2 PROC
    mov cr2, rcx
    ret
__writecr2 ENDP


__cli PROC
    cli
    ret
__cli ENDP


__sti PROC
    sti
    ret
__sti ENDP


__pause PROC
    pause
    ret
__pause ENDP

END
