[bits 32]

extern KernelInit

global _start
_start:
    mov byte [0xB8000], 'A'
    xchg bx, bx
    call KernelInit
    xchg bx, bx
    jmp $