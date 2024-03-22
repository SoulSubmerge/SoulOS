[bits 32]
; 中断处理函数入口 

extern handlerTable
extern taskSignal

section .text

%macro INTERRUPT_HANDLER 2
INTERRUPT_HANDLER_FN_%1:
    ; xchg bx, bx
%ifn %2
    push 0x4F5DA2
%endif
    push %1; 压入中断向量，跳转到中断入口
    jmp interruptEntryFn
%endmacro

interruptEntryFn:

    ; 保存上文寄存器信息
    push ds
    push es
    push fs
    push gs
    pushad

    ; 找到前面 push %1 压入的 中断向量
    mov eax, [esp + 12 * 4]

    ; 向中断处理函数传递参数
    push eax

    ; 调用中断处理函数，handler_table 中存储了中断处理函数的指针
    call [handlerTable + eax * 4]

global interruptExitFn
interruptExitFn:
    ; 对应 push eax，调用结束恢复栈
    add esp, 4
    ; 调用信号处理函数
    ; call taskSignal

    ; 恢复下文寄存器信息
    popad
    pop gs
    pop fs
    pop es
    pop ds

    ; 对应 push %1
    ; 对应 error code 或 push magic
    add esp, 8
    iret

INTERRUPT_HANDLER 0x00, 0; divide by zero
INTERRUPT_HANDLER 0x01, 0; debug
INTERRUPT_HANDLER 0x02, 0; non maskable interrupt
INTERRUPT_HANDLER 0x03, 0; breakpoint

INTERRUPT_HANDLER 0x04, 0; overflow
INTERRUPT_HANDLER 0x05, 0; bound range exceeded
INTERRUPT_HANDLER 0x06, 0; invalid opcode
INTERRUPT_HANDLER 0x07, 0; device not avilable

INTERRUPT_HANDLER 0x08, 1; double fault
INTERRUPT_HANDLER 0x09, 0; coprocessor segment overrun
INTERRUPT_HANDLER 0x0a, 1; invalid TSS
INTERRUPT_HANDLER 0x0b, 1; segment not present

INTERRUPT_HANDLER 0x0c, 1; stack segment fault
INTERRUPT_HANDLER 0x0d, 1; general protection fault
INTERRUPT_HANDLER 0x0e, 1; page fault
INTERRUPT_HANDLER 0x0f, 0; reserved

INTERRUPT_HANDLER 0x10, 0; x87 floating point exception
INTERRUPT_HANDLER 0x11, 1; alignment check
INTERRUPT_HANDLER 0x12, 0; machine check
INTERRUPT_HANDLER 0x13, 0; SIMD Floating - Point Exception

INTERRUPT_HANDLER 0x14, 0; Virtualization Exception
INTERRUPT_HANDLER 0x15, 1; Control Protection Exception
INTERRUPT_HANDLER 0x16, 0; reserved
INTERRUPT_HANDLER 0x17, 0; reserved

INTERRUPT_HANDLER 0x18, 0; reserved
INTERRUPT_HANDLER 0x19, 0; reserved
INTERRUPT_HANDLER 0x1a, 0; reserved
INTERRUPT_HANDLER 0x1b, 0; reserved

INTERRUPT_HANDLER 0x1c, 0; reserved
INTERRUPT_HANDLER 0x1d, 0; reserved
INTERRUPT_HANDLER 0x1e, 0; reserved
INTERRUPT_HANDLER 0x1f, 0; reserved

INTERRUPT_HANDLER 0x20, 0; clock 时钟中断
INTERRUPT_HANDLER 0x21, 0; keyboard 键盘中断
INTERRUPT_HANDLER 0x22, 0; cascade 级联 8259
INTERRUPT_HANDLER 0x23, 0; com2 串口2
INTERRUPT_HANDLER 0x24, 0; com1 串口1
INTERRUPT_HANDLER 0x25, 0; sb16 声霸卡
INTERRUPT_HANDLER 0x26, 0; floppy 软盘
INTERRUPT_HANDLER 0x27, 0
INTERRUPT_HANDLER 0x28, 0; rtc 实时时钟
INTERRUPT_HANDLER 0x29, 0
INTERRUPT_HANDLER 0x2a, 0
INTERRUPT_HANDLER 0x2b, 0; nic 网卡
INTERRUPT_HANDLER 0x2c, 0
INTERRUPT_HANDLER 0x2d, 0
INTERRUPT_HANDLER 0x2e, 0; harddisk1 硬盘主通道
INTERRUPT_HANDLER 0x2f, 0; harddisk2 硬盘从通道

; 下面的数组记录了每个中断入口函数的指针
section .data
global handlerEntryTable
handlerEntryTable:
    dd INTERRUPT_HANDLER_FN_0x00
    dd INTERRUPT_HANDLER_FN_0x01
    dd INTERRUPT_HANDLER_FN_0x02
    dd INTERRUPT_HANDLER_FN_0x03
    dd INTERRUPT_HANDLER_FN_0x04
    dd INTERRUPT_HANDLER_FN_0x05
    dd INTERRUPT_HANDLER_FN_0x06
    dd INTERRUPT_HANDLER_FN_0x07
    dd INTERRUPT_HANDLER_FN_0x08
    dd INTERRUPT_HANDLER_FN_0x09
    dd INTERRUPT_HANDLER_FN_0x0a
    dd INTERRUPT_HANDLER_FN_0x0b
    dd INTERRUPT_HANDLER_FN_0x0c
    dd INTERRUPT_HANDLER_FN_0x0d
    dd INTERRUPT_HANDLER_FN_0x0e
    dd INTERRUPT_HANDLER_FN_0x0f
    dd INTERRUPT_HANDLER_FN_0x10
    dd INTERRUPT_HANDLER_FN_0x11
    dd INTERRUPT_HANDLER_FN_0x12
    dd INTERRUPT_HANDLER_FN_0x13
    dd INTERRUPT_HANDLER_FN_0x14
    dd INTERRUPT_HANDLER_FN_0x15
    dd INTERRUPT_HANDLER_FN_0x16
    dd INTERRUPT_HANDLER_FN_0x17
    dd INTERRUPT_HANDLER_FN_0x18
    dd INTERRUPT_HANDLER_FN_0x19
    dd INTERRUPT_HANDLER_FN_0x1a
    dd INTERRUPT_HANDLER_FN_0x1b
    dd INTERRUPT_HANDLER_FN_0x1c
    dd INTERRUPT_HANDLER_FN_0x1d
    dd INTERRUPT_HANDLER_FN_0x1e
    dd INTERRUPT_HANDLER_FN_0x1f
    dd INTERRUPT_HANDLER_FN_0x20
    dd INTERRUPT_HANDLER_FN_0x21
    dd INTERRUPT_HANDLER_FN_0x22
    dd INTERRUPT_HANDLER_FN_0x23
    dd INTERRUPT_HANDLER_FN_0x24
    dd INTERRUPT_HANDLER_FN_0x25
    dd INTERRUPT_HANDLER_FN_0x26
    dd INTERRUPT_HANDLER_FN_0x27
    dd INTERRUPT_HANDLER_FN_0x28
    dd INTERRUPT_HANDLER_FN_0x29
    dd INTERRUPT_HANDLER_FN_0x2a
    dd INTERRUPT_HANDLER_FN_0x2b
    dd INTERRUPT_HANDLER_FN_0x2c
    dd INTERRUPT_HANDLER_FN_0x2d
    dd INTERRUPT_HANDLER_FN_0x2e
    dd INTERRUPT_HANDLER_FN_0x2f

section .text

extern syscallCheck
extern syscallTable
global syscallHandler
syscallHandler:
    ; xchg bx, bx

    ; 验证系统调用号
    push eax
    call syscallCheck
    ; add esp, 4
    pop eax
    push 0x20222202

    push 0x80

    ; 保存上文寄存器信息
    push ds
    push es
    push fs
    push gs
    pushad

    push 0x80; 向中断处理函数传递参数中断向量 vector


    ; push ebp; 第六个参数
    ; push edi; 第五个参数
    ; push esi; 第四个参数
    push edx; 第三个参数
    push ecx; 第二个参数
    push ebx; 第一个参数

    ; 调用系统调用处理函数，syscall_table 中存储了系统调用处理函数的指针
    call [syscallTable + eax * 4]
    add esp, (3 * 4); 系统调用结束恢复栈
    ; 修改栈中 eax 寄存器，设置系统调用返回值
    mov dword [esp + 8 * 4], eax

    ; 跳转到中断返回
    jmp interruptExitFn

; 设置 IF 位
global setInterruptStateTrue
setInterruptStateTrue:
    sti
    ret

global setInterruptStateFalse
setInterruptStateFalse:
    cli
    ret

global interruptDisable
interruptDisable:
    pushfd
    cli
    pop eax
    shr eax, 0x09
    and eax, 0x01
    ret

global getInterruptState
getInterruptState:
    pushfd
    pop eax
    shr eax, 0x09
    and eax, 0x01
    ret


