.CODE

AsmCpuPause PROC
    pause
    ret
AsmCpuPause ENDP

AsmReadCs PROC
    xor rax, rax
    mov ax, cs
    ret
AsmReadCs ENDP

AsmReadDs PROC
    xor rax, rax
    mov ax, ds
    ret
AsmReadDs ENDP

AsmReadSs PROC
    xor rax, rax
    mov ax, ss
    ret
AsmReadSs ENDP

AsmReadEs PROC
    xor rax, rax
    mov ax, es
    ret
AsmReadEs ENDP

AsmReadFs PROC
    xor rax, rax
    mov ax, fs
    ret
AsmReadFs ENDP

AsmReadGs PROC
    xor rax, rax
    mov ax, gs
    ret
AsmReadGs ENDP

AsmLimitFromSelector PROC
    lsl rax, rcx
    ret
AsmLimitFromSelector ENDP

AsmReadTr PROC
    xor rax, rax
    str ax
    ret
AsmReadTr ENDP

AsmReadGdtr PROC
    sgdt fword ptr [rcx]
    ret
AsmReadGdtr ENDP

AsmReadLdtr PROC
    xor rax, rax
    sldt eax
    ret
AsmReadLdtr ENDP

AsmReadFlags PROC
    pushfq
    pop rax
    ret
AsmReadFlags ENDP

AsmWriteCs PROC
    mov cs, cx
    ret
AsmWriteCs ENDP

AsmWriteDs PROC
    mov ds, cx
    ret
AsmWriteDs ENDP

AsmWriteSs PROC
    mov ss, cx
    ret
AsmWriteSs ENDP

AsmWriteEs PROC
    mov es, cx
    ret
AsmWriteEs ENDP

AsmWriteFs PROC
    mov fs, cx
    ret
AsmWriteFs ENDP

AsmWriteGs PROC
    mov gs, cx
    ret
AsmWriteGs ENDP

AsmWriteTr PROC
    ltr cx
    ret
AsmWriteTr ENDP

AsmReadSp PROC
    mov rax, rsp
    ret
AsmReadSp ENDP

AsmWriteGdtr PROC
    lgdt fword ptr [rcx]
    ret
AsmWriteGdtr ENDP

AsmEnableInterrupts PROC
    sti
    ret
AsmEnableInterrupts ENDP

AsmDisableInterrupts PROC
    cli
    ret
AsmDisableInterrupts ENDP

END
