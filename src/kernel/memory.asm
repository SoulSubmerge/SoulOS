[bits 32]

section .text

global getCr2
getCr2:
    mov eax, cr2
    ret

global getCr3
getCr3:
    mov eax, cr3
    ret

global _setCr3
_setCr3:
    mov eax, [esp + 4]
    mov cr3, eax
    ret

; 将 cr0 寄存器最高位 PG 置为 1，启用分页
global enablePage
enablePage:
    mov eax, cr0
    or eax, 0x80000000 ; 0b1000_0000_0000_0000_0000_0000_0000_0000
    mov cr0, eax
    ret

; 刷新块表
global flushTlb
flushTlb:
    mov ebx, [esp + 4]
    invlpg [ebx]
    ret