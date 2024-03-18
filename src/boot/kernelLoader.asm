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

; 进入保护模式代码
PrepareProtectedMode:
    cli ; 关闭中断
    ; 打开 A20 总线
    in al, 0x92
    or al, 0b10
    out 0x92, al
    lgdt [gdtPtr]
    ; 启动保护模式
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp dword CODE_SELECTOR: (0x1000 + ProtectMode)




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

[bits 32]
ProtectMode:
    mov eax, DATA_SELECTOR
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x10000
    
    ; 读取内核代码
    ; 0x100000 - 0xAFFFFF 为内核的栈空间 10MB
    ; 0xB00000 - 0x6efffff 为内核的代码 100MB
    ; 0x6f00000 - 2G 数据空间
    ; push 0x05
    ; push 0xB00000
    ; push 0xc8 ; 200
    ; call ReadDiskFn
    ; add esp, 0x0c
    push 0x05
    push 0xB00000
    push 0x50 ; 0x50 -> 80块 10MB ; 0x190 -> 400 块 50MB
    call ReadBlocksFn
    add esp, 0x0c
    ; pop eax
    ; pop eax
    ; pop eax
    jmp dword CODE_SELECTOR:0xB00000

; 由于端口一次性只能读取 256 个扇区，所以需要建 256 个扇区作为一个块来读
ReadBlocksFn:
    mov ecx, [esp + 4] ; 读取的块数
    .ReadNextBlocFn:
        ; 计算起始的扇区号
        mov eax, [esp + 12]
        mov ebx, [esp + 4]
        ; xchg bx, bx
        cmp ebx, ecx
        jz .BreakFn
        add eax, 0x100
        mov [esp + 12], eax
        .BreakFn:
        push eax
        ; 计算读取数据到目标的内存地址
        mov eax, [esp + 12] ; 因为前面又push的参数，所以加12
        cmp ebx, ecx
        jz .BreakTwoFn
        add eax, 0x20000 ; eax + 512*256
        mov [esp + 12], eax
        .BreakTwoFn:
        push eax
        ; 计算读取的扇区数
        push 0x100
        call ReadDiskFn
        add esp, 0x0c
        loop .ReadNextBlocFn
    ret

; 磁盘读取代码
ReadDiskFn:
    push ecx
    ; xchg bx, bx
    mov ebx, [esp + 12] ; 读取数据到目标的内存地址
    mov ecx, [esp + 8] ; 读取的扇区数

    ; 设置读写扇区的数量
    mov dx, 0x1f2
    mov al, cl
    out dx, al

    mov eax, [esp + 16] ; 读取的起始扇区编号

    inc dx; 0x1f3 起始扇区的前八位
    out dx, al

    inc dx; 0x1f4 起始扇区的中八位
    shr eax, 8
    out dx, al

    inc dx; 0x1f5 起始扇区的高八位
    shr eax, 8
    out dx, al

    inc dx; 0x1f6
    shr eax, 8
    and eax, 0x000F; 将高四位置为 0
    or eax, 0x00E0 ;
    out dx, al; 主盘 - LBA 模式

    inc dx; 0x1f7
    mov eax, 0x20; 读硬盘
    out dx, al

    .ReadFn:
        push ecx
        call .WaitsFn
        call .readsFn
        pop ecx
        loop .ReadFn
    pop ecx
    ; xchg bx, bx
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
        mov cx, 256; 一个扇区 256 字, 1字 = 2byte
        .ReadWordFn:
            in ax, dx
            jmp $+2; 一点点延迟
            jmp $+2
            jmp $+2
            mov [ebx], ax
            add ebx, 2
            ; cmp cx, 0
            loop .ReadWordFn
        ret



CODE_SELECTOR equ (1 << 3)
DATA_SELECTOR equ (2 << 3)

MEMORY_BASE equ 0 ; 内存的基地址

; 内存界限 4G / 4K - 1
MEMORY_LIMIT equ ((1024 * 1024 * 1024 * 4) / (1024 * 4)) - 1

gdtPtr:
    dw (gdtEnd - gdtBase) - 1
    dd 0x1000 + gdtBase
gdtBase:
    dd 0, 0; NULL 描述符
gdtCode:
    dw MEMORY_LIMIT & 0xffff; 段界限 0 ~ 15 位
    dw MEMORY_BASE & 0xffff; 基地址 0 ~ 15 位
    db (MEMORY_BASE >> 16) & 0xff; 基地址 16 ~ 23 位
    ; 存在 - dlp 0 - S _ 代码 - 非依从 - 可读 - 没有被访问过
    db 0b_1_00_1_1_0_1_0;
    ; 4k - 32 位 - 不是 64 位 - 段界限 16 ~ 19
    db 0b1_1_0_0_0000 | (MEMORY_LIMIT >> 16) & 0xf;
    db (MEMORY_BASE >> 24) & 0xff; 基地址 24 ~ 31 位
gdtData:
    dw MEMORY_LIMIT & 0xffff; 段界限 0 ~ 15 位
    dw MEMORY_BASE & 0xffff; 基地址 0 ~ 15 位
    db (MEMORY_BASE >> 16) & 0xff; 基地址 16 ~ 23 位
    ; 存在 - dlp 0 - S _ 数据 - 向上 - 可写 - 没有被访问过
    db 0b_1_00_1_0_0_1_0;
    ; 4k - 32 位 - 不是 64 位 - 段界限 16 ~ 19
    db 0b1_1_0_0_0000 | (MEMORY_LIMIT >> 16) & 0xf;
    db (MEMORY_BASE >> 24) & 0xff; 基地址 24 ~ 31 位
gdtEnd: