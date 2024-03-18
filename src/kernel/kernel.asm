[bits 32]

extern kernelInit

global _start
_start:
    mov esp, 0xAFFFFF
    call kernelInit
    int 0x08
    jmp $