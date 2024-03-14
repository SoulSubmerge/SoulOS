[org 0x0000] ; 0x1000


; 1. 从boot将kernelLoader代码加载进内存
; 2. 实现内存检测和相关的存储结构的构建
; 3. 进入保护模式
dw 0x55aa ; 魔术，用于修改

; 初始化段寄存器
mov ax, 0x100 ; 因为主引导扇区的主引导代码一开始会被载入带内存的 0x7c00 的位置
mov ds, ax
xor ax, ax 
mov es, ax
mov ss, ax
mov sp, 0x1000

mov si, kernelLoadingMessage
call PrintFn

jmp $

; 打印相关的文本信息到屏幕
PrintFn:
    mov ah, 0x0e
    .next:
        mov al, [si] ; 实际的计算是 [段寄存器 << 4 + si]， si的默认组合是和 ds
        cmp al, 0x00
        jz .done
        int 0x10
        inc si
        jmp .next
    .done:
        ret


kernelLoadingMessage:
    db "The SoulOS kernel loader code is loaded.", 0x0a, 0x0d, 0x00 ; 