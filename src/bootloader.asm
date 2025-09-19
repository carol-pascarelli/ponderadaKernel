[bits 32]           ; 32 bits
[extern kernel_main]

section .text
global _start

_start:
    ; chama o kernel em C
    call kernel_main

    ; loop infinito (caso kernel retorne)
.hang:
    jmp .hang
