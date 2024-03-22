[bits 32]

magic   equ 0xe85250d6
i386    equ 0
length  equ header_end - header_start

section .multiboot2
header_start:
    dd magic  ; 魔数
    dd i386   ; 32位保护模式
    dd length ; 头部长度
    dd -(magic + i386 + length); 校验和

    ; 结束标记
    dw 0    ; type
    dw 0    ; flags
    dd 8    ; size
header_end:

extern consoleInit
extern gdtInit
extern memoryInit
extern kernelInit
extern m_gdtPtr ; 内核的全局描述符指针
code_selector equ (1 << 3)
data_selector equ (2 << 3)

section .text
global _start
_start:
    mov esp, 0xAFFFFF
    push ebx; 内存检测结果的结构数量指针
    push eax; magic
    ; 内存检测的初始化函数
    call consoleInit
    ; xchg bx, bx
    call gdtInit
    ; xchg bx, bx
    lgdt [m_gdtPtr]
    jmp dword code_selector:_next

_next:
    mov ax, data_selector
    mov ds, ax
    mov es, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    ; mov esp, 0xAFFFFF
    call memoryInit
    add esp, 0x08
    call kernelInit
    ; xchg bx, bx
    ; mov eax, 0 ; 0 号系统调用
    ; int 0x80
    jmp $