[org 0x0000]

; 编译指令： nasm -f bin boot.asm -o boot.bin
; 设置屏幕模式为文本模式，清除屏幕
mov ax, 0x0003 ; AH=0x00 设置模式 AL=0x03 文字 80*25 颜色2
int 0x10

; 初始化段寄存器
mov ax, 0x7c0 ; 因为主引导扇区的主引导代码一开始会被载入带内存的 0x7c00 的位置
mov ds, ax
xor ax, ax 

; 再次查看github是否ok
; 测试提交效果
mov es, ax
mov ss, ax
mov sp, 0x7c00

; 显示文本
xchg bx, bx ; 魔术断点
mov si, message
call print


; 阻塞
jmp $

; 打印相关的文本信息到屏幕
print:
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

message:
    db "Loading SoulOS ... ...", 0 ; 

times 510 - ($ - $$) db 0x00

; 硬盘最后两个字节必须是 0x55 0xaa
db 0x55, 0xaa