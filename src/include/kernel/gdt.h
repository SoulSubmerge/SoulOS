#ifndef SOUL_GDT_H
#define SOUL_GDT_H
#include <types/types.h>
#include <kernel/debug.h>

#define GDT_SIZE 128

#define KERNEL_CODE_INDEX 1
#define KERNEL_DATA_INDEX 2
#define KERNEL_TSS_INDEX 3

#define USER_CODE_INDEX 4
#define USER_DATA_INDEX 5

#define KERNEL_CODE_SELECTOR (KERNEL_CODE_INDEX << 3)
#define KERNEL_DATA_SELECTOR (KERNEL_DATA_INDEX << 3)
#define KERNEL_TSS_SELECTOR (KERNEL_TSS_INDEX << 3)

#define USER_CODE_SELECTOR (USER_CODE_INDEX << 3 | 0b11)
#define USER_DATA_SELECTOR (USER_DATA_INDEX << 3 | 0b11)

#pragma pack(1)
// 全局描述符表
typedef struct gdt_descriptor
{
    uint16 limitLow;      // 段界限 0 ~ 15 位
    uint32 baseLow : 24;    // 基地址 0 ~ 23 位 16M
    uint8 type : 4;        // 段类型
    uint8 segment : 1;     // 1 表示代码段或数据段，0 表示系统段
    uint8 DPL : 2;         // Descriptor Privilege Level 描述符特权等级 0 ~ 3
    uint8 present : 1;     // 存在位，1 在内存中，0 在磁盘上
    uint8 limitHigh : 4;  // 段界限 16 ~ 19;
    uint8 available : 1;   // 该安排的都安排了，送给操作系统吧
    uint8 longMode : 1;   // 64 位扩展标志
    uint8 big : 1;         // 32 位 还是 16 位;
    uint8 granularity : 1; // 粒度 4KB 或 1B
    uint8 baseHigh;       // 基地址 24 ~ 31 位
}GDT_DESCRIPTOR;
#pragma pack()

#pragma pack(1)
// 全局描述符指针
typedef struct gdt_pointer
{
    uint16 limit;
    uint32 basePtr;
}GDT_POINTER;
#pragma pack()

#pragma pack(1)
// 段选择子
typedef struct gdt_selector
{
    uint8 RPL : 2;
    uint8 TI : 1;
    uint16 index : 13;
}GDT_SELECTOR;
#pragma pack()

#pragma pack(1)
typedef struct tss_t
{
    uint32 backlink; // 前一个任务的链接，保存了前一个任状态段的段选择子
    uint32 esp0;     // ring0 的栈顶地址
    uint32 ss0;      // ring0 的栈段选择子
    uint32 esp1;     // ring1 的栈顶地址
    uint32 ss1;      // ring1 的栈段选择子
    uint32 esp2;     // ring2 的栈顶地址
    uint32 ss2;      // ring2 的栈段选择子
    uint32 cr3;
    uint32 eip;
    uint32 flags;
    uint32 eax;
    uint32 ecx;
    uint32 edx;
    uint32 ebx;
    uint32 esp;
    uint32 ebp;
    uint32 esi;
    uint32 edi;
    uint32 es;
    uint32 cs;
    uint32 ss;
    uint32 ds;
    uint32 fs;
    uint32 gs;
    uint32 ldtr;          // 局部描述符选择子
    uint16 trace : 1;     // 如果置位，任务切换时将引发一个调试异常
    uint16 reversed : 15; // 保留不用
    uint16 iobase;        // I/O 位图基地址，16 位从 TSS 到 IO 权限位图的偏移
    uint32 ssp;           // 任务影子栈指针
}TSS_T;
#pragma pack()

#endif