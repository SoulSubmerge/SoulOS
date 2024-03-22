#include <kernel/gdt.h>
#include <lib/charArray.h>
#include <kernel/debug.h>
#include <kernel/assert.h>


GDT_DESCRIPTOR m_gdt[GDT_SIZE];
GDT_POINTER m_gdtPtr;
TSS_T m_tss;

// Gdt::~Gdt(){};

void setMemory(GDT_DESCRIPTOR *desc, uint32 base, uint32 limit)
{
    desc->baseLow = base & 0xffffff;
    desc->baseHigh = (base >> 24) & 0xff;
    desc->limitLow = limit & 0xffff;
    desc->limitHigh = (limit >> 16) & 0xf;
}

// index 1  内核代码段
// index 2  内核数据段
// index 3  tss
// index 4  用户代码段
// index 5  用户数据段
extern "C" void gdtInit()
{
    memset(&m_gdt[0], 0, sizeof(GDT_DESCRIPTOR));
    memset(&m_tss, 0, sizeof(TSS_T));
    DEBUGK("init gdt!!!\n");
    DEBUGK("gdt address: %p", &m_gdt[0]);
    DEBUGK("gdt_pointer address: %p", &m_gdtPtr);
    DEBUGK("tss address: %p", &m_tss);
    memset(m_gdt, 0, sizeof(m_gdt));
    GDT_DESCRIPTOR *gdtDesPtr = nullptr;
    gdtDesPtr = &m_gdt[KERNEL_CODE_INDEX];
    setMemory(gdtDesPtr, 0, 0xFFFFF);
    gdtDesPtr->segment = 1;     // 代码段
    gdtDesPtr->granularity = 1; // 4K
    gdtDesPtr->big = 1;         // 32 位
    gdtDesPtr->longMode = 0;   // 不是 64 位
    gdtDesPtr->present = 1;     // 在内存中
    gdtDesPtr->DPL = 0;         // 内核特权级
    gdtDesPtr->type = 0b1010;   // 代码 / 非依从 / 可读 / 没有被访问过

    gdtDesPtr = &m_gdt[KERNEL_DATA_INDEX];
    setMemory(gdtDesPtr, 0, 0xFFFFF);
    gdtDesPtr->segment = 1;     // 数据段
    gdtDesPtr->granularity = 1; // 4K
    gdtDesPtr->big = 1;         // 32 位
    gdtDesPtr->longMode = 0;   // 不是 64 位
    gdtDesPtr->present = 1;     // 在内存中
    gdtDesPtr->DPL = 0;         // 内核特权级
    gdtDesPtr->type = 0b0010;   // 数据 / 向上增长 / 可写 / 没有被访问过

    // tss
    m_tss.ss0 = KERNEL_DATA_SELECTOR;
    m_tss.iobase = sizeof(m_tss);
    gdtDesPtr = &m_gdt[KERNEL_TSS_INDEX];
    setMemory(gdtDesPtr, (uint32)&m_tss, sizeof(m_tss) - 1);
    gdtDesPtr->segment = 0;     // 系统段
    gdtDesPtr->granularity = 0; // 字节
    gdtDesPtr->big = 0;         // 固定为 0
    gdtDesPtr->longMode = 0;   // 固定为 0
    gdtDesPtr->present = 1;     // 在内存中
    gdtDesPtr->DPL = 0;         // 用于任务门或调用门
    gdtDesPtr->type = 0b1001;   // 32 位可用 tss


    gdtDesPtr = &m_gdt[USER_CODE_INDEX];
    setMemory(gdtDesPtr, 0, 0xFFFFF);
    gdtDesPtr->segment = 1;     // 代码段
    gdtDesPtr->granularity = 1; // 4K
    gdtDesPtr->big = 1;         // 32 位
    gdtDesPtr->longMode = 0;   // 不是 64 位
    gdtDesPtr->present = 1;     // 在内存中
    gdtDesPtr->DPL = 3;         // 用户特权级
    gdtDesPtr->type = 0b1010;   // 代码 / 非依从 / 可读 / 没有被访问过

    gdtDesPtr = &m_gdt[USER_DATA_INDEX];
    setMemory(gdtDesPtr, 0, 0xFFFFF);
    gdtDesPtr->segment = 1;     // 数据段
    gdtDesPtr->granularity = 1; // 4K
    gdtDesPtr->big = 1;         // 32 位
    gdtDesPtr->longMode = 0;   // 不是 64 位
    gdtDesPtr->present = 1;     // 在内存中
    gdtDesPtr->DPL = 3;         // 用户特权级
    gdtDesPtr->type = 0b0010;   // 数据 / 向上增长 / 可写 / 没有被访问过

    m_gdtPtr.basePtr = (uint32)&m_gdt;
    m_gdtPtr.limit = sizeof(m_gdt) - 1;
    // asm volatile("lgdt %0\n" :: "m" (m_gdtPtr));
}


void tssInit()
{
    memset(&m_tss, 0, sizeof(m_tss));
    m_tss.ss0 = KERNEL_DATA_SELECTOR;
    m_tss.iobase = sizeof(m_tss);
    GDT_DESCRIPTOR *desc = m_gdt + KERNEL_TSS_INDEX;
    setMemory(desc, (uint32)&m_tss, sizeof(m_tss) - 1);
    desc->segment = 0;     // 系统段
    desc->granularity = 0; // 字节
    desc->big = 0;         // 固定为 0
    desc->longMode = 0;   // 固定为 0
    desc->present = 1;     // 在内存中
    desc->DPL = 0;         // 用于任务门或调用门
    desc->type = 0b1001;   // 32 位可用 tss
    // BMB;
    asm volatile(
        "ltr %%ax\n" ::"a"(KERNEL_TSS_SELECTOR));
}