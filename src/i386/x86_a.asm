.686p
.MODEL flat, C

.CODE

AsmCpuPause@0 PROC
    pause
    ret
AsmCpuPause@0 ENDP

AsmReadCs@0 PROC
    xor eax, eax
    mov ax, cs
    ret
AsmReadCs@0 ENDP

AsmReadDs@0 PROC
    xor eax, eax
    mov ax, ds
    ret
AsmReadDs@0 ENDP

AsmReadSs@0 PROC
    xor eax, eax
    mov ax, ss
    ret
AsmReadSs@0 ENDP

AsmReadEs@0 PROC
    xor eax, eax
    mov ax, es
    ret
AsmReadEs@0 ENDP

AsmReadFs@0 PROC
    xor eax, eax
    mov ax, fs
    ret
AsmReadFs@0 ENDP

AsmReadGs@0 PROC
    xor eax, eax
    mov ax, gs
    ret
AsmReadGs@0 ENDP

AsmLimitFromSelector@4 PROC
    mov ecx, [esp + 4]
    lsl eax, ecx
    ret 4
AsmLimitFromSelector@4 ENDP

AsmReadTr@0 PROC
    xor eax, eax
    str ax
    ret
AsmReadTr@0 ENDP

AsmReadGdtr@4 PROC
    mov eax, [esp + 4]
    sgdt [eax]
    ret 4
AsmReadGdtr@4 ENDP

AsmReadLdtr@0 PROC
    xor eax, eax
    sldt ax
    ret
AsmReadLdtr@0 ENDP

AsmReadFlags@0 PROC
    pushfd
    pop eax
    ret
AsmReadFlags@0 ENDP

AsmWriteDs@4 PROC
    mov eax, [esp + 4]
    mov ds, ax
    ret 4
AsmWriteDs@4 ENDP

AsmWriteSs@4 PROC
    mov eax, [esp + 4]
    mov ss, ax
    ret 4
AsmWriteSs@4 ENDP

AsmWriteEs@4 PROC
    mov eax, [esp + 4]
    mov es, ax
    ret 4
AsmWriteEs@4 ENDP

AsmWriteFs@4 PROC
    mov eax, [esp + 4]
    mov fs, ax
    ret 4
AsmWriteFs@4 ENDP

AsmWriteGs@4 PROC
    mov eax, [esp + 4]
    mov gs, ax
    ret 4
AsmWriteGs@4 ENDP

AsmWriteTr@4 PROC
    mov eax, [esp + 4]
    ltr ax
    ret 4
AsmWriteTr@4 ENDP

AsmReadSp@0 PROC
    mov eax, esp
    ret
AsmReadSp@0 ENDP

AsmWriteCr2@4 PROC
    mov eax, [esp + 4]
    mov cr2, eax
    ret 4
AsmWriteCr2@4 ENDP

AsmWriteGdtr@4 PROC
    mov eax, [esp + 4]
    lgdt fword ptr [eax]
    ret 4
AsmWriteGdtr@4 ENDP

AsmEnableInterrupts@0 PROC
    sti
    ret
AsmEnableInterrupts@0 ENDP

AsmDisableInterrupts@0 PROC
    cli
    ret
AsmDisableInterrupts@0 ENDP

END
