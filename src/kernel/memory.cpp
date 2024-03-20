#include <kernel/memory.h>
#include <kernel/kernel.h>
#include <kernel/logk.h>
#include <kernel/assert.h>
#include <lib/stdlib.h>
#include <lib/charArray.h>
#include <lib/bitmap.h>
#include <kernel/debug.h>

#ifdef SOUL_DEBUG
#define USER_MEMORY true
#else
#define USER_MEMORY false
#endif

#define ZONE_VALID 1    // ards 可用内存区域
#define ZONE_RESERVED 2 // ards 不可用区域




#define IDX(addr) ((uint32)addr >> 12) // 获取 addr 的页索引
#define DIDX(addr) (((uint32)addr >> 22) & 0x3ff) // 获取 addr 的页目录索引
#define TIDX(addr) (((uint32)addr >> 12) & 0x3ff) // 获取 addr 的页表索引
#define PAGE(idx) ((uint32)idx << 12)             // 获取页索引 idx 对应的页开始的位置
#define ASSERT_PAGE(addr) assert((addr & 0xfff) == 0, "The memory page has no memory alignment.")

#define PDE_MASK 0xFFC00000

BITMAP_T kernelMap;

// 内核页表索引
static uint32 KERNEL_PAGE_TABLE[] = {
    0x2000,
    0x3000,
    0x4000,
    0x5000,
};

#define KERNEL_MAP_BITS 0x6000

static uint32 memoryBase = 0; // 可用内存基地址，应该等于 1M
static uint32 memorySize = 0; // 可用内存大小
static uint32 totalPages = 0; // 所有内存页数
static uint32 freePages = 0;  // 空闲内存页数

#define usedPages (totalPages - freePages) // 已用页数

extern "C"  void memoryInit(uint32 magic, uint32 addr)
{
    uint32 count = 0;
    MEM_ADDR_T *addrPtr = nullptr;

    // 如果是 onix loader 进入的内核
    if (magic == SOUL_MAGIC)
    {
        LOGK("Log address struct...%p\n", addr);
        count = *(uint32*)addr;
        addrPtr = (MEM_ADDR_T *)(addr + 4);
        for (uint32 i = 0; i < count; i++, addrPtr++)
        {
            LOGK("Memory base 0x%p - 0x%p size 0x%p type %d\n",
                 (uint32)addrPtr->base, (uint32)addrPtr->base + (uint32)addrPtr->size - 1, (uint32)addrPtr->size, (uint32)addrPtr->type);
            if (addrPtr->type == ZONE_VALID && addrPtr->size > memorySize)
            {
                memoryBase = (uint32)addrPtr->base;
                memorySize = (uint32)addrPtr->size;
            }
        }
    }
    else
    {
        panic("Memory init magic unknown 0x%p\n", magic);
    }

    LOGK("ARDS count %d\n", count);
    LOGK("Memory base 0x%p\n", (uint32)memoryBase);
    LOGK("Memory size 0x%p\n", (uint32)memorySize);

    assert(memoryBase == MEMORY_BASE, "The memory start position is incorrect."); // 内存开始的位置为 1M
    assert((memorySize & 0xfff) == 0, "Memory page alignment error."); // 要求按页对齐

    totalPages = IDX(memorySize) + IDX(MEMORY_BASE);
    freePages = IDX(memorySize);

    LOGK("Total pages %d\n", totalPages);
    LOGK("Free pages %d\n", freePages);

    if (memorySize < KERNEL_MEMORY_SIZE)
    {
        panic("System memory is %dM too small, at least %dM needed\n",
              memorySize / MEMORY_BASE, KERNEL_MEMORY_SIZE / MEMORY_BASE);
    }
}

static uint32 startPage = 0;   // 可分配物理内存起始位置
static uint8 *memoryMap;       // 物理内存数组
static uint32 memoryMapPages; // 物理内存数组占用的页数

void memoryMapInit()
{
    // 初始化物理内存数组
    memoryMap = (uint8*)memoryBase;

    // 计算物理内存数组占用的页数
    memoryMapPages = divRoundUp(totalPages, PAGE_SIZE);
    LOGK("Memory map page count %d\n", memoryMapPages);

    freePages -= memoryMapPages;

    // 清空物理内存数组
    memset((void *)memoryMap, 0, memoryMapPages * PAGE_SIZE);

    // 前 1M 的内存位置 以及 物理内存数组已占用的页，已被占用
    startPage = IDX(MEMORY_BASE) + memoryMapPages;
    for (size_t i = 0; i < startPage; i++)
    {
        memoryMap[i] = 1;
    }

    LOGK("Total pages %d free pages %d\n", totalPages, freePages);

    // 初始化内核虚拟内存位图，需要 8 位对齐
    // uint32 length = (IDX(KERNEL_RAMDISK_MEM) - IDX(MEMORY_BASE)) / 8;
    uint32 length = (IDX(KERNEL_MEMORY_SIZE * sizeof(KERNEL_PAGE_TABLE)) - IDX(MEMORY_BASE)) / 8;;
    
    bitmapInit(&kernelMap, (char*)KERNEL_MAP_BITS, length, IDX(MEMORY_BASE));
    bitmapScan(&kernelMap, memoryMapPages);
}

// 分配一页物理内存
static uint32 getPage()
{
    for (size_t i = startPage; i < totalPages; i++)
    {
        // 如果物理内存没有占用
        if (!memoryMap[i])
        {
            memoryMap[i] = 1;
            assert(freePages > 0, "No memory pages are available.");
            freePages--;
            uint32 page = PAGE(i);
            LOGK("GET page 0x%p\n", page);
            return page;
        }
    }
    panic("Out of Memory!!!");
    return UINT32_MAX;
}


// 释放一页物理内存
static void putPage(uint32 addr)
{
    ASSERT_PAGE(addr); // 内存对齐检测

    uint32 idx = IDX(addr);

    // idx 大于 1M 并且 小于 总页面数
    assert(idx >= startPage && idx < totalPages, "Error obtaining memory page");

    // 保证只有一个引用
    assert(memoryMap[idx] >= 1, "A physical reference to a memory page is duplicated.");

    // 物理引用减一
    memoryMap[idx]--;

    // 若为 0，则空闲页加一
    if (!memoryMap[idx])
    {
        freePages++;
    }

    assert(freePages > 0 && freePages < totalPages, "Memory page error");
    LOGK("PUT page 0x%p\n", addr);
}

extern "C" void _setCr3(uint32 pde); // 代码汇编实现了
extern "C" void enablePage(); // 开启分页，将cr0 最高位置为 1，代码汇编实现的

// 设置 cr3 寄存器，参数是页目录的地址
void setCr3(uint32 pde)
{
    ASSERT_PAGE(pde);
    _setCr3(pde);
}

// 初始化页表项
static void entryInit(PAGE_ENTRY_T *entry, uint32 index)
{
    *(uint32*)entry = 0;
    entry->present = 1;
    entry->write = 1;
    entry->user = 1;
    entry->index = index;
}

// 初始化内存映射
void mappingInit()
{
    PAGE_ENTRY_T *pde = (PAGE_ENTRY_T*)KERNEL_PAGE_DIR;
    memset(pde, 0, PAGE_SIZE);

    idx_t index = 0;
    for (idx_t didx = 0; didx < (sizeof(KERNEL_PAGE_TABLE) / 4); didx++)
    {
        PAGE_ENTRY_T *pte = (PAGE_ENTRY_T*)KERNEL_PAGE_TABLE[didx];
        memset(pte, 0, PAGE_SIZE);

        PAGE_ENTRY_T *dentry = &pde[didx];
        entryInit(dentry, IDX((uint32)pte));
        dentry->user = USER_MEMORY; // 只能被内核访问
        LOGK("PET Address: %p didx: %d\n", dentry, didx);
        for (idx_t tidx = 0; tidx < 1024; tidx++, index++)
        {
            // 第 0 页不映射，为造成空指针访问，缺页异常，便于排错
            if (index == 0)
                continue;
            PAGE_ENTRY_T *tentry = &pte[tidx];
            entryInit(tentry, index);
            tentry->user = USER_MEMORY; // 只能被内核访问
            if (memoryMap[index] == 0)
                freePages--;
            memoryMap[index] = 1; // 设置物理内存数组，该页被占用
        }
    }

    // // 将最后一个页表指向页目录自己，方便修改
    PAGE_ENTRY_T *entry = &pde[1023];
    entryInit(entry, IDX(KERNEL_PAGE_DIR));
    // 设置 cr3 寄存器
    setCr3((uint32)pde);
    // 分页有效
    enablePage();
    // BREAK_POINT;
}

// 获取页目录
static PAGE_ENTRY_T *getPde()
{
    return (PAGE_ENTRY_T *)(0xfffff000);
}

// 获取虚拟地址 vaddr 对应的页表
static PAGE_ENTRY_T *getPte(uint32 vaddr, bool create)
{
    // PAGE_ENTRY_T *pde = getPde();
    // uint32 idx = DIDX(vaddr);
    // PAGE_ENTRY_T *entry = &pde[idx];

    // assert(create || (!create && entry->present), "Accessing an illegal memory page.");

    // PAGE_ENTRY_T *table = (PAGE_ENTRY_T *)(PDE_MASK | (idx << 12));

    // if (!entry->present)
    // {
    //     LOGK("Get and create page table entry for 0x%p\n", vaddr);
    //     uint32 page = getPage();
    //     entryInit(entry, IDX(page));
    //     memset(table, 0, PAGE_SIZE);
    // }
    // return table;

     return (page_entry_t *)(PDE_MASK | (DIDX(vaddr) << 12));
}

// 从位图中扫描 count 个连续的页
static uint32 scanPage(BITMAP_T *map, uint32 count)
{
    assert(count > 0, "The number of application pages is illegal.");
    int32 index = bitmapScan(map, count);

    if (index == EOF)
    {
        panic("Scan page fail!!!");
    }

    uint32 addr = PAGE(index);
    LOGK("Scan page 0x%p count %d\n", addr, count);
    return addr;
}

// 与 scan_page 相对，重置相应的页
static void resetPage(BITMAP_T *map, uint32 addr, uint32 count)
{
    ASSERT_PAGE(addr);
    assert(count > 0, "The number of pages destroyed is illegal");
    uint32 index = IDX(addr);

    for (size_t i = 0; i < count; i++)
    {
        assert(bitmapTest(map, index + i), "Destroy unused pages.");
        bitmapSet(map, index + i, 0);
    }
}

// 分配 count 个连续的内核页
uint32 allocKpage(uint32 count)
{
    assert(count > 0, "The number of application pages is illegal.");
    uint32 vaddr = scanPage(&kernelMap, count);
    LOGK("ALLOC kernel pages 0x%p count %d\n", vaddr, count);
    return vaddr;
}

// 释放 count 个连续的内核页
void freeKpage(uint32 vaddr, uint32 count)
{
    ASSERT_PAGE(vaddr);
    assert(count > 0, "Destroy unused pages.");
    resetPage(&kernelMap, vaddr, count);
    LOGK("FREE  kernel pages 0x%p count %d\n", vaddr, count);
}

void memoryTest()
{
    uint32 *pages = (uint32 *)(0x200000);
    uint32 count = 0x6fe;
    for (size_t i = 0; i < count; i++)
    {
        pages[i] = allocKpage(1);
        LOGK("0x%x\n", i);
    }
    for (size_t i = 0; i < count; i++)
    {
        freeKpage(pages[i], 1);
    }
}