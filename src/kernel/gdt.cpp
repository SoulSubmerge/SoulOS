#include <kernel/gdt.h>
#include <lib/charArray.h>
#include <kernel/debug.h>
#include <kernel/assert.h>


Gdt::Gdt()
{
    memset(&this->m_gdt[0], 0, sizeof(GDT_DESCRIPTOR));
    memset(&this->m_tss, 0, sizeof(TSS_T));
}

// Gdt::~Gdt(){};

void Gdt::setMemory(uint16 _index, uint32 _base, uint32 _limit)
{
    assert(_index < GDT_SIZE, "gdt selector out of bounds");
    this->m_gdt[_index].baseLow = _base & 0xffffff;
    this->m_gdt[_index].baseHigh = (_base >> 24) & 0xff;
    this->m_gdt[_index].limitLow = _limit & 0xffff;
    this->m_gdt[_index].limitHigh = (_limit >> 16) & 0xf;
}

// index 1  内核代码段
// index 2  内核数据段
// index 3  tss
// index 4  用户代码段
// index 5  用户数据段
void Gdt::init()
{
    DEBUGK("init gdt!!!\n");
    DEBUGK("address: %p", this);
    DEBUGK("gdt address: %p", &this->m_gdt[0]);
    DEBUGK("gdt_pointer address: %p", &this->m_gdtPtr);
    DEBUGK("tss address: %p", &this->m_tss);
    GDT_DESCRIPTOR *gdtDesPtr = nullptr;
    gdtDesPtr = &this->m_gdt[KERNEL_CODE_INDEX];
    this->setMemory(KERNEL_CODE_INDEX, 0, 0xFFFFF);
    gdtDesPtr->segment = 1;     // 代码段
    gdtDesPtr->granularity = 1; // 4K
    gdtDesPtr->big = 1;         // 32 位
    gdtDesPtr->longMode = 0;   // 不是 64 位
    gdtDesPtr->present = 1;     // 在内存中
    gdtDesPtr->DPL = 0;         // 内核特权级
    gdtDesPtr->type = 0b1010;   // 代码 / 非依从 / 可读 / 没有被访问过

    gdtDesPtr = &this->m_gdt[KERNEL_DATA_INDEX];
    this->setMemory(KERNEL_DATA_INDEX, 0, 0xFFFFF);
    gdtDesPtr->segment = 1;     // 数据段
    gdtDesPtr->granularity = 1; // 4K
    gdtDesPtr->big = 1;         // 32 位
    gdtDesPtr->longMode = 0;   // 不是 64 位
    gdtDesPtr->present = 1;     // 在内存中
    gdtDesPtr->DPL = 0;         // 内核特权级
    gdtDesPtr->type = 0b0010;   // 数据 / 向上增长 / 可写 / 没有被访问过

    // tss
    this->m_tss.ss0 = KERNEL_DATA_SELECTOR;
    this->m_tss.iobase = sizeof(this->m_tss);
    gdtDesPtr = &this->m_gdt[KERNEL_TSS_INDEX];
    this->setMemory(KERNEL_TSS_INDEX, (uint32)&this->m_tss, sizeof(this->m_tss) - 1);
    gdtDesPtr->segment = 0;     // 系统段
    gdtDesPtr->granularity = 0; // 字节
    gdtDesPtr->big = 0;         // 固定为 0
    gdtDesPtr->longMode = 0;   // 固定为 0
    gdtDesPtr->present = 1;     // 在内存中
    gdtDesPtr->DPL = 0;         // 用于任务门或调用门
    gdtDesPtr->type = 0b1001;   // 32 位可用 tss


    gdtDesPtr = &this->m_gdt[USER_CODE_INDEX];
    this->setMemory(USER_CODE_INDEX, 0, 0xFFFFF);
    gdtDesPtr->segment = 1;     // 代码段
    gdtDesPtr->granularity = 1; // 4K
    gdtDesPtr->big = 1;         // 32 位
    gdtDesPtr->longMode = 0;   // 不是 64 位
    gdtDesPtr->present = 1;     // 在内存中
    gdtDesPtr->DPL = 3;         // 用户特权级
    gdtDesPtr->type = 0b1010;   // 代码 / 非依从 / 可读 / 没有被访问过

    gdtDesPtr = &this->m_gdt[USER_DATA_INDEX];
    this->setMemory(USER_DATA_INDEX, 0, 0xFFFFF);
    gdtDesPtr->segment = 1;     // 数据段
    gdtDesPtr->granularity = 1; // 4K
    gdtDesPtr->big = 1;         // 32 位
    gdtDesPtr->longMode = 0;   // 不是 64 位
    gdtDesPtr->present = 1;     // 在内存中
    gdtDesPtr->DPL = 3;         // 用户特权级
    gdtDesPtr->type = 0b0010;   // 数据 / 向上增长 / 可写 / 没有被访问过

    this->m_gdtPtr.basePtr = (uint32)&this->m_gdt;
    this->m_gdtPtr.limit = sizeof(this->m_gdt) - 1;
    asm volatile("lgdt %0\n" :: "m" (this->m_gdtPtr));
    DEBUGK("init gdt ok!!!\n");
    asm volatile(
        "ltr %%ax\n" ::"a"(KERNEL_TSS_SELECTOR));
}

static Gdt globalGdt;
extern "C" void gdtInit()
{
    globalGdt.init();
}