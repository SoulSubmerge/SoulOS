[bits 32]

extern kernelInit

global _start
_start:
    mov byte [0xB8000], 'A'
    call kernelInit
    jmp $