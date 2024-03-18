#include <kernel/interrupt.h>
#include <io/io.h>
#include <kernel/assert.h>
#include <kernel/debug.h>
#include <kernel/printk.h>
#include <kernel/task.h>

IDT_DESCRIPTOR globalIdt[IDT_SIZE];
IDT_POINTER globalIdtPtr;

extern "C" void interruptHandler();

#define LOGK(fmt, args...) DEBUGK(fmt, ##args)
// #define LOGK(fmt, args...)

#define ENTRY_SIZE 0x30

handleFunc handlerTable[IDT_SIZE];
extern handleFunc handlerEntryTable[ENTRY_SIZE];
extern "C" void syscallHandler();
extern "C" void pageFault();

static const char *unexpectedMessages[] = {
    "#DE Divide Error\0",
    "#DB RESERVED\0",
    "--  NMI Interrupt\0",
    "#BP Breakpoint\0",
    "#OF Overflow\0",
    "#BR BOUND Range Exceeded\0",
    "#UD Invalid Opcode (Undefined Opcode)\0",
    "#NM Device Not Available (No Math Coprocessor)\0",
    "#DF Double Fault\0",
    "    Coprocessor Segment Overrun (reserved)\0",
    "#TS Invalid TSS\0",
    "#NP Segment Not Present\0",
    "#SS Stack-Segment Fault\0",
    "#GP General Protection\0",
    "#PF Page Fault\0",
    "--  (Intel reserved. Do not use.)\0",
    "#MF x87 FPU Floating-Point Error (Math Fault)\0",
    "#AC Alignment Check\0",
    "#MC Machine Check\0",
    "#XF SIMD Floating-Point Exception\0",
    "#VE Virtualization Exception\0",
    "#CP Control Protection Exception\0",
};

// 通知中断控制器，中断处理结束
void sendEoi(int32 _vector)
{
    if (_vector >= 0x20 && _vector < 0x28)
    {
        outByte(PIC_M_CTRL, PIC_EOI);
    }
    if (_vector >= 0x28 && _vector < 0x30)
    {
        outByte(PIC_M_CTRL, PIC_EOI);
        outByte(PIC_S_CTRL, PIC_EOI);
    }
}


// 注册异常处理函数
void setExceptionHandler(uint32 _index, handleFunc _handler)
{
    assert(_index >= 0 && _index <= 17, "The subscript of the registered exception handler is out of bounds. ");
    handlerTable[_index] = _handler;
}

// 注册中断处理函数
void setInterruptHandler(uint32 _irq, handleFunc _handler)
{
    assert(_irq >= 0 && _irq < 16, "The subscript of the registered interrupt handler is out of bounds. ");
    handlerTable[IRQ_MASTER_NR + _irq] = _handler;
}

void setInterruptMask(uint32 _irq, bool _enable)
{
    assert(_irq >= 0 && _irq < 16, "The subscript of the registration interrupt handler mask is out of bounds.");
    uint16 port;
    if (_irq < 8)
    {
        port = PIC_M_DATA;
    }
    else
    {
        port = PIC_S_DATA;
        _irq -= 8;
    }
    if (_enable)
    {
        outByte(port, inByte(port) & ~(1 << _irq));
    }
    else
    {
        outByte(port, inByte(port) | (1 << _irq));
    }
}

void setInterruptState(bool _state)
{
    if(_state)
    {
        setInterruptStateTrue();
    }
    else
    {
        setInterruptStateFalse();
    }
}
extern void scheduler();
void defaultHandler(int32 _vector)
{
    sendEoi(_vector);
    DEBUGK("[%x] default interrupt called...\n", _vector);
    // scheduler();
}

void exceptionHandler(
    int32 vector,
    uint32 edi, uint32 esi, uint32 ebp, uint32 esp,
    uint32 ebx, uint32 edx, uint32 ecx, uint32 eax,
    uint32 gs, uint32 fs, uint32 es, uint32 ds,
    uint32 vector0, uint32 error, uint32 eip, uint32 cs, uint32 eflags)
{
    const char *message = nullptr;
    if (vector < 22)
    {
        message = unexpectedMessages[vector];
    }
    else
    {
        message = unexpectedMessages[15];
    }

    printk("\nEXCEPTION : %s \n", message);
    printk("   VECTOR : 0x%02X\n", vector);
    printk("    ERROR : 0x%08X\n", error);
    printk("   EFLAGS : 0x%08X\n", eflags);
    printk("       CS : 0x%02X\n", cs);
    printk("      EIP : 0x%08X\n", eip);
    printk("      ESP : 0x%08X\n", esp);

    // 阻塞
    while (true);
    // 通过 EIP 的值应该可以找到出错的位置
    // 也可以在出错时，可以将 hanging 在调试器中手动设置为 0
    // 然后在下面 return 打断点，单步调试，找到出错的位置
    return;
}

// 初始化中断控制器
void picInit()
{
    outByte(PIC_M_CTRL, 0b00010001); // ICW1: 边沿触发, 级联 8259, 需要ICW4.
    outByte(PIC_M_DATA, 0x20);       // ICW2: 起始中断向量号 0x20
    outByte(PIC_M_DATA, 0b00000100); // ICW3: IR2接从片.
    outByte(PIC_M_DATA, 0b00000001); // ICW4: 8086模式, 正常EOI

    outByte(PIC_S_CTRL, 0b00010001); // ICW1: 边沿触发, 级联 8259, 需要ICW4.
    outByte(PIC_S_DATA, 0x28);       // ICW2: 起始中断向量号 0x28
    outByte(PIC_S_DATA, 2);          // ICW3: 设置从片连接到主片的 IR2 引脚
    outByte(PIC_S_DATA, 0b00000001); // ICW4: 8086模式, 正常EOI

    outByte(PIC_M_DATA, 0b11111110); // 关闭所有中断
    outByte(PIC_S_DATA, 0b11111111); // 关闭所有中断
}

void idtInit()
{
    for (uint32 i = 0; i < ENTRY_SIZE; i++)
    {
        IDT_DESCRIPTOR *tempIdtPtr = &globalIdt[i];
        handleFunc tempHandler = handlerEntryTable[i];

        tempIdtPtr->offsetLow = (uint32)tempHandler & 0xffff;
        tempIdtPtr->offsetHigh = ((uint32)tempHandler >> 16) & 0xffff;
        tempIdtPtr->codeSelector = 1 << 3; // 代码段
        tempIdtPtr->reserved = 0;      // 保留不用
        tempIdtPtr->type = 0b1110;     // 中断门
        tempIdtPtr->segment = 0;       // 系统段
        tempIdtPtr->DPL = 0;           // 内核态
        tempIdtPtr->present = 1;       // 有效
    }

    for (uint32 i = 0; i < 0x20; i++)
    {
        handlerTable[i] = (void*)exceptionHandler;
    }

    // handlerTable[0xe] = (void*)pageFault;

    for (uint32 i = 0x20; i < ENTRY_SIZE; i++)
    {
        handlerTable[i] = (void*)defaultHandler;
    }

    // // 初始化系统调用
    // IDT_DESCRIPTOR *tempIdtPtr = &globalIdt[0x80];
    // tempIdtPtr->offsetLow = (uint32)syscallHandler & 0xffff;
    // tempIdtPtr->offsetHigh = ((uint32)syscallHandler >> 16) & 0xffff;
    // tempIdtPtr->codeSelector = 1 << 3; // 代码段
    // tempIdtPtr->reserved = 0;      // 保留不用
    // tempIdtPtr->type = 0b1110;     // 中断门
    // tempIdtPtr->segment = 0;       // 系统段
    // tempIdtPtr->DPL = 3;           // 用户态
    // tempIdtPtr->present = 1;       // 有效

    globalIdtPtr.basePtr = (uint32)globalIdt;
    globalIdtPtr.limit = sizeof(globalIdt) - 1;
    asm volatile("lidt globalIdtPtr\n");
}

void interruptInit()
{
    picInit();
    idtInit();
}