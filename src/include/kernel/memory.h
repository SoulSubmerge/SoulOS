#ifndef SOUL_MEMORY_H
#define SOUL_MEMORY_H
#include <types/types.h>

#define PAGE_SIZE 0x1000     // 一页的大小 4K
#define MEMORY_BASE 0x100000 // 1M，可用内存开始的位置

// 内核占用的内存大小 8M
#define KERNEL_MEMORY_SIZE 0x800000

// 内存虚拟磁盘地址 20M后
#define KERNEL_RAMDISK_MEM 0x8000000


// 用户栈顶地址 256M
#define USER_STACK_TOP 0x8000000

// 用户栈最大 2M
#define USER_STACK_SIZE 0x200000

// 用户栈底地址
#define USER_STACK_BOTTOM (USER_STACK_TOP - USER_STACK_SIZE)

// 内核页目录索引
#define KERNEL_PAGE_DIR 0x1000

#pragma pack(1)
typedef struct ards_t
{
    uint64 base; // 内存基地址
    uint64 size; // 内存长度
    uint32 type; // 类型
}ards_t;
#pragma pack()

#pragma pack(1)
typedef struct page_entry_t
{
    uint8 present : 1;  // 在内存中
    uint8 write : 1;    // 0 只读 1 可读可写
    uint8 user : 1;     // 1 所有人 0 超级用户 DPL < 3
    uint8 pwt : 1;      // page write through 1 直写模式，0 回写模式
    uint8 pcd : 1;      // page cache disable 禁止该页缓冲
    uint8 accessed : 1; // 被访问过，用于统计使用频率
    uint8 dirty : 1;    // 脏页，表示该页缓冲被写过
    uint8 pat : 1;      // page attribute table 页大小 4K/4M
    uint8 global : 1;   // 全局，所有进程都用到了，该页不刷新缓冲
    // uint8 shared : 1;   // 共享内存页，与 CPU 无关
    // uint8 privat : 1;   // 私有内存页，与 CPU 无关
    // uint8 readonly : 1; // 只读内存页，与 CPU 无关
    uint8 ignored : 3;
    uint32 index : 20;  // 页索引
}page_entry_t;
#pragma pack()


#pragma pack(1)
typedef struct page_error_code_t
{
    uint8 present : 1;
    uint8 write : 1;
    uint8 user : 1;
    uint8 reserved0 : 1;
    uint8 fetch : 1;
    uint8 protection : 1;
    uint8 shadow : 1;
    uint16 reserved1 : 8;
    uint8 sgx : 1;
    uint16 reserved2;
}page_error_code_t;
#pragma pack()

extern "C" uint32 getCr2(); // 得到 cr2 寄存器，代码汇编实现了
extern "C" uint32 getCr3(); // 得到 cr3 寄存器，代码汇编实现了
extern "C" void setCr3(uint32 pde); // 设置 cr3 寄存器，参数是页目录的地址
extern "C" void flushTlb(uint32 vaddr); // 刷新块表

uint32 allocKpage(uint32 count); // 分配 count 个连续的内核页
void freeKpage(uint32 vaddr, uint32 count); // 释放 count 个连续的内核页

// 将 vaddr 映射物理内存
void linkPage(uint32 vaddr);

// 去掉 vaddr 对应的物理内存映射
void unlinkPage(uint32 vaddr);

// 拷贝页目录
page_entry_t *copyPde();

// 释放页目录
void freePde();

// 系统调用 brk
int32 sysBrk(void *addr);

#endif