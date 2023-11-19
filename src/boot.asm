[org 0x7c00]

; 编译指令： nasm -f bin boot.asm -o boot.bin
; 设置屏幕模式为文本模式，清除屏幕
mov ax, 3
int 0x10

; 初始化段寄存器
xor ax, ax
mov ds, ax
mov es, ax
mov ss, ax
mov sp, 0x7c00

; 0xb8000 文本显示器的内存区域
mov ax, 0xb800
mov ds, ax
mov byte [0], 'H'


; 阻塞
jmp $

times 510 - ($ - $$) db 0x00

; 硬盘最后两个字节必须是 0x55 0xaa
db 0x55, 0xaa