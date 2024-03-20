[bits 32]

extern consoleInit
extern gdtInit
extern memoryInit
extern kernelInit

global _start
_start:
    mov esp, 0xAFFFFF
    push ebx; 内存检测结果的结构数量指针
    push eax; magic
    ; 内存检测的初始化函数
    call consoleInit
    call gdtInit
    call memoryInit 
    call kernelInit
    ; xchg bx, bx
    ; int 0x08
    jmp $