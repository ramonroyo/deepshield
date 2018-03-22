BITS 64

global rdpmc_memory_access
global memory_access_time
global detect_vmware
global __vmware_tsc

section .text

__vmware_tsc:
  mov rcx, 10000h
  rdpmc
  shl rdx, 20h
  or rax, rdx
  ret

detect_vmware:
  xor rax, rax
  mov rax, 564D5868h
  mov rbx, 0
  mov rcx, 0Ah
  mov rdx, 5658h
  in eax, dx
  cmp rbx, 564D5868h
  je detected
  xor rax, rax
  jmp no_detected
detected:
  mov rax, 1
no_detected:
  ret

rdpmc_memory_access:
  mov r9, rcx
  mov r10, rdx
  mov rcx, r10
  rdpmc
  shl rdx, 20h
  or rax, rdx
  mov r8, rax

  mov rax, [r9] ; memory access

  mov rcx, r10
  rdpmc
  shl rdx, 20h
  or rax, rdx

  sub rax,r8
  ret

memory_access_time:
  mov r9, rcx

  rdtscp
  shl rdx, 20h
  or rax, rdx
  mov r8, rax

  mov rax, [r9]

  rdtscp
  shl rdx, 20h
  or rax, rdx

  sub rax,r8
  ret
