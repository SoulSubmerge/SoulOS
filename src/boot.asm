[org 0x0000]

; 编译指令： nasm -f bin boot.asm -o boot.bin
; 设置屏幕模式为文本模式，清除屏幕
mov ax, 0x0003 ; AH=0x00 设置模式 AL=0x03 文字 80*25 颜色2
int 0x10

; 初始化段寄存器
mov ax, 0x7c0 ; 因为主引导扇区的主引导代码一开始会被载入带内存的 0x7c00 的位置
mov ds, ax
xor ax, ax 
mov es, ax
mov ss, ax
mov sp, 0x7c00

; booting 相关信息显示
xchg bx, bx ; 魔术断点
mov si, bootingMessage
call PrintFn


; 内核加载器读取
mov ax, es
push ax
mov ax, 0x00
mov es, ax ; 设置附加段寄存器值为0
mov edi, 0x1000; 读取的目标内存
mov ecx, 1; 起始扇区
mov bl, 4; 扇区数量
call ReadDiskFn
pop ax
mov es, ax


mov ax, es
push ax
mov ax, 0x00
mov es, ax
cmp word [es:0x1000], 0x55aa ; 检测内核加载器的代码是否正确读取到内存了
pop ax
mov es, ax
jnz bootingErrorOut
mov si, bootingMessage
call PrintFn

; 阻塞
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

ReadDiskFn:
    ; 设置读写扇区的数量
    mov dx, 0x1f2
    mov al, bl
    out dx, al

    inc dx; 0x1f3
    mov al, cl; 起始扇区的前八位
    out dx, al

    inc dx; 0x1f4
    shr ecx, 8
    mov al, cl; 起始扇区的中八位
    out dx, al

    inc dx; 0x1f5
    shr ecx, 8
    mov al, cl; 起始扇区的高八位
    out dx, al

    inc dx; 0x1f6
    shr ecx, 8
    and cl, 0b1111; 将高四位置为 0

    mov al, 0b1110_0000;
    or al, cl
    out dx, al; 主盘 - LBA 模式

    inc dx; 0x1f7
    mov al, 0x20; 读硬盘
    out dx, al

    xor ecx, ecx; 将 ecx 清空
    mov cl, bl; 得到读写扇区的数量

    .ReadFn:
        push cx ;
        call .WaitsFn ;
        call .readsFn ;
        pop cx ;
        loop .ReadFn
    ret

    .WaitsFn:
        mov dx, 0x1f7
        .CheckFn:
            in al, dx
            jmp $+2; nop 直接跳转到下一行
            jmp $+2; 一点点延迟
            jmp $+2
            and al, 0b1000_1000
            cmp al, 0b0000_1000
            jnz .CheckFn
        ret
    
    .readsFn:
        mov dx, 0x1f0
        mov cx, 256; 一个扇区 256 字
        .ReadWordFn:
            in ax, dx
            jmp $+2; 一点点延迟
            jmp $+2
            jmp $+2
            mov [es:edi], ax
            add edi, 2
            loop .ReadWordFn
        ret

bootingMessage:
    db "The SoulOS operating system main boot sector has been loaded.",0x0a, 0x0d, 0 ; \n\r

bootingErrorOut:
    mov si, bootingErrorMsg
    call PrintFn
    htl ; 要 CPU 休眠
    jmp $

bootingErrorMsg:
    db "booting failed to read the disk.", 0x0a, 0x0d, 0 ;

times 510 - ($ - $$) db 0x00

; 硬盘最后两个字节必须是 0x55 0xaa
db 0x55, 0xaa