[org 0x7c00]

; 编译指令： nasm -f bin boot.asm -o boot.bin
; 设置屏幕模式为文本模式，清除屏幕
mov ax, 0x0003 ; AH=0x00 设置模式 AL=0x03 文字 80*25 颜色2
int 0x10

; 初始化段寄存器
xor ax, ax
mov ds, ax
mov es, ax
mov ss, ax
mov sp, 0x7c00

; 显示文本
mov ah, 0x0E
mov al, 'S'
int 0x10
mov al, 'o'
int 0x10
mov al, 'u'
int 0x10
mov al, 'l'
int 0x10
mov al, 'O'
int 0x10
mov al, 'S'
int 0x10


; 阻塞
jmp $

times 510 - ($ - $$) db 0x00

; 硬盘最后两个字节必须是 0x55 0xaa
db 0x55, 0xaa