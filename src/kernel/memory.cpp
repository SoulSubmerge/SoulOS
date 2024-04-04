#include <kernel/memory.h>
#include <kernel/kernel.h>
#include <kernel/logk.h>
#include <kernel/assert.h>
#include <lib/stdlib.h>
#include <lib/charArray.h>
#include <lib/bitmap.h>
#include <kernel/debug.h>
#include <kernel/multiboot2.h>
#include <kernel/task.h>

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

bitmap_t kernelMap;

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

    // 如果是 onix loader 进入的内核
    if (magic == SOUL_MAGIC)
    {
        count = *(uint32*)addr;
        ards_t *addrPtr = (ards_t *)(addr + 4);
        for (uint32 i = 0; i < count; i++, addrPtr++)
        {
            LOGK("Memory base 0x%p - 0x%p size 0x%p type %d\n",(uint32)addrPtr->base, (uint32)addrPtr->base + (uint32)addrPtr->size - 1, (uint32)addrPtr->size, (uint32)addrPtr->type);
            if (addrPtr->type == ZONE_VALID && addrPtr->size > memorySize)
            {
                memoryBase = (uint32)addrPtr->base;
                memorySize = (uint32)addrPtr->size;
            }
        }
    }
    else if(magic == MULTIBOOT2_MAGIC)
    {
        uint32 size = *(unsigned int *)addr;
        multi_tag_t *tag = (multi_tag_t *)(addr + 8);

        while (tag->type != MULTIBOOT_TAG_TYPE_END)
        {
            if (tag->type == MULTIBOOT_TAG_TYPE_MMAP)
                break;
            // 下一个 tag 对齐到了 8 字节
            tag = (multi_tag_t *)((uint32)tag + ((tag->size + 7) & ~7));
        }

        multi_tag_mmap_t *mtag = (multi_tag_mmap_t *)tag;
        multi_mmap_entry_t *entry = mtag->entries;
        while ((uint32)entry < (uint32)tag + tag->size)
        {
            LOGK("Memory base 0x%p size 0x%p type %d\n",
                 (uint32)entry->addr, (uint32)entry->len, (uint32)entry->type);
            count++;
            if (entry->type == ZONE_VALID && entry->len > memorySize)
            {
                memoryBase = (uint32)entry->addr;
                memorySize = (uint32)entry->len;
            }
            entry = (multi_mmap_entry_t *)((uint32)entry + mtag->entry_size);
        }
    }
    else
    {
        panic("Memory init magic unknown 0x%p\n", magic);
    }

    LOGK("ARDS count %d", count);
    LOGK("Memory base 0x%p", (uint32)memoryBase);
    LOGK("Memory size 0x%p", (uint32)memorySize);

    assert(memoryBase == MEMORY_BASE, "The memory start position is incorrect."); // 内存开始的位置为 1M
    assert((memorySize & 0xfff) == 0, "Memory page alignment error."); // 要求按页对齐

    totalPages = IDX(memorySize) + IDX(MEMORY_BASE);
    freePages = IDX(memorySize);

    LOGK("Total pages %d", totalPages);
    LOGK("Free pages %d", freePages);

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
    LOGK("Memory map page count %d", memoryMapPages);

    freePages -= memoryMapPages;

    // 清空物理内存数组
    memset((void *)memoryMap, 0, memoryMapPages * PAGE_SIZE);

    // 前 1M 的内存位置 以及 物理内存数组已占用的页，已被占用
    startPage = IDX(MEMORY_BASE) + memoryMapPages;
    for (size_t i = 0; i < startPage; i++)
    {
        memoryMap[i] = 1;
    }

    LOGK("Total pages %d free pages %d", totalPages, freePages);

    // 初始化内核虚拟内存位图，需要 8 位对齐
    uint32 length = (IDX(KERNEL_RAMDISK_MEM) - IDX(MEMORY_BASE)) / 8;
    // uint32 length = (IDX(KERNEL_MEMORY_SIZE * sizeof(KERNEL_PAGE_TABLE)) - IDX(MEMORY_BASE)) / 8;;
    
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
            freePages--;
            assert(freePages >= 0, "No memory pages are available.");
            uint32 page = PAGE(i);
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
}

extern "C" void _setCr3(uint32 pde); // 代码汇编实现了
extern "C" void enablePage(); // 开启分页，将cr0 最高位置为 1，代码汇编实现的

// 设置 cr3 寄存器，参数是页目录的地址
extern "C" void setCr3(uint32 pde)
{
    // ASSERT_PAGE(pde);
    _setCr3(pde);
    // asm volatile("movl %%eax, %%cr3\n" ::"a"(pde));
}

// 初始化页表项
static void entryInit(page_entry_t *entry, uint32 index)
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
    page_entry_t *pde = (page_entry_t*)KERNEL_PAGE_DIR;
    memset(pde, 0, PAGE_SIZE);

    idx_t index = 0;
    for (idx_t didx = 0; didx < (sizeof(KERNEL_PAGE_TABLE) / 4); didx++)
    {
        page_entry_t *pte = (page_entry_t*)KERNEL_PAGE_TABLE[didx];
        memset(pte, 0, PAGE_SIZE);

        page_entry_t *dentry = &pde[didx];
        entryInit(dentry, IDX((uint32)pte));
        dentry->user = USER_MEMORY; // 只能被内核访问

        for (idx_t tidx = 0; tidx < 1024; tidx++, index++)
        {
            // 第 0 页不映射，为造成空指针访问，缺页异常，便于排错
            if (index == 0)
                continue;
            page_entry_t *tentry = &pte[tidx];
            entryInit(tentry, index);
            // tentry->user = USER_MEMORY; // 只能被内核访问
            // if (memoryMap[index] == 0)
            //     freePages--;
            memoryMap[index] = 1; // 设置物理内存数组，该页被占用
        }
    }

    // // 将最后一个页表指向页目录自己，方便修改
    page_entry_t *entry = &pde[1023];
    entryInit(entry, IDX(KERNEL_PAGE_DIR));
    // 设置 cr3 寄存器
    setCr3((uint32)pde);
    // 分页有效
    enablePage();
}

// 获取页目录
static page_entry_t *getPde()
{
    return (page_entry_t *)(0xfffff000);
}

// 获取虚拟地址 vaddr 对应的页表
static page_entry_t *getPte(uint32 vaddr, bool create)
{
    page_entry_t *pde = getPde();
    uint32 idx = DIDX(vaddr);
    page_entry_t *entry = &pde[idx];

    assert(create || (!create && entry->present), "Accessing an illegal memory page.");

    page_entry_t *table = (page_entry_t *)(PDE_MASK | (idx << 12));

    if (!entry->present)
    {
        uint32 page = getPage();
        entryInit(entry, IDX(page));
        memset(table, 0, PAGE_SIZE);
    }
    return table;
}

// 从位图中扫描 count 个连续的页
static uint32 scanPage(bitmap_t *map, uint32 count)
{
    assert(count > 0, "The number of application pages is illegal.");
    int32 index = bitmapScan(map, count);

    if (index == EOF)
    {
        panic("Scan page fail!!!");
    }

    uint32 addr = PAGE(index);
    return addr;
}

// 与 scan_page 相对，重置相应的页
static void resetPage(bitmap_t *map, uint32 addr, uint32 count)
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
    return vaddr;
}

// 释放 count 个连续的内核页
void freeKpage(uint32 vaddr, uint32 count)
{
    ASSERT_PAGE(vaddr);
    assert(count > 0, "Destroy unused pages.");
    resetPage(&kernelMap, vaddr, count);
}

void memoryTest()
{
    uint32 *pages = (uint32 *)(0x200000);
    uint32 count = 0x6fe;
    for (size_t i = 0; i < count; i++)
    {
        pages[i] = allocKpage(1);
    }
    for (size_t i = 0; i < count; i++)
    {
        freeKpage(pages[i], 1);
    }
}

// 将 vaddr 映射物理内存
void linkPage(uint32 vaddr)
{
    ASSERT_PAGE(vaddr);

    page_entry_t *pte = getPte(vaddr, true);
    page_entry_t *entry = &pte[TIDX(vaddr)];

    task_t *task = runningTask();
    bitmap_t *map = task->vmap;
    uint32 index = IDX(vaddr);

    // 如果页面已存在，则直接返回
    if (entry->present)
    {
        assert(bitmapTest(map, index), "Expect the memory page to exist, but it does not.");
        return;
    }

    assert(!bitmapTest(map, index), "Expect the memory page not to exist, but it does not.");
    bitmapSet(map, index, true);

    uint32 paddr = getPage();
    entryInit(entry, IDX(paddr));
    flushTlb(vaddr);

}

// 去掉 vaddr 对应的物理内存映射
void unlinkPage(uint32 vaddr)
{
    ASSERT_PAGE(vaddr);

    page_entry_t *pte = getPte(vaddr, true);
    page_entry_t *entry = &pte[TIDX(vaddr)];

    task_t *task = runningTask();
    bitmap_t *map = task->vmap;
    uint32 index = IDX(vaddr);

    if (!entry->present)
    {
        assert(!bitmapTest(map, index), "Expect the memory page not to exist, but it does not.");
        return;
    }

    assert(entry->present && bitmapTest(map, index), "Expect the memory page to exist, but it does not.");

    entry->present = false;
    bitmapSet(map, index, false);

    uint32 paddr = PAGE(entry->index);

    DEBUGK("UNLINK from 0x%p to 0x%p", vaddr, paddr);
    putPage(paddr);
    flushTlb(vaddr);
}

// 拷贝一页，返回拷贝后的物理地址
static uint32 copyPage(void *page)
{
    uint32 paddr = getPage();

    page_entry_t *entry = getPte(0, false);
    entryInit(entry, IDX(paddr));
    memcpy((void *)0, (void *)page, PAGE_SIZE);

    entry->present = false;
    return paddr;
}


// 拷贝当前页目录
page_entry_t *copyPde()
{
    task_t *task = runningTask();
    page_entry_t *pde = (page_entry_t *)allocKpage(1); // todo free

    memcpy(pde, (void *)task->pde, PAGE_SIZE);
    // 将最后一个页表指向页目录自己，方便修改
    page_entry_t *entry = &pde[1023];
    entryInit(entry, IDX(pde));
    page_entry_t *dentry = nullptr;

    for (size_t didx = (sizeof(KERNEL_PAGE_TABLE) / 4); didx < 1023; didx++)
    {
        dentry = &pde[didx];
        if (!dentry->present)
            continue;

        page_entry_t *pte = (page_entry_t *)(PDE_MASK | (didx << 12));
        for (size_t tidx = 0; tidx < 1024; tidx++)
        {
            entry = &pte[tidx];
            if (!entry->present)
                continue;

            // 对应物理内存引用大于 0
            assert(memoryMap[entry->index] > 0, "Index overshoot.");
            // 置为只读
            entry->write = false;
            // 对应物理页引用加 1
            memoryMap[entry->index]++;

            assert(memoryMap[entry->index] < 255, "Index overshoot.");
        }

        uint32 paddr = copyPage(pte);
        dentry->index = IDX(paddr);
    }
    setCr3(task->pde);

    return pde;
}

// 释放当前页目录
void freePde()
{
    task_t *task = runningTask();
    assert(task->uid != KERNEL_USER, "Permission error, expected kernel mode.");

    page_entry_t *pde = getPde();

    for (size_t didx = (sizeof(KERNEL_PAGE_TABLE) / 4); didx < 1023; didx++)
    {
        page_entry_t *dentry = &pde[didx];
        if (!dentry->present)
        {
            continue;
        }

        page_entry_t *pte = (page_entry_t *)(PDE_MASK | (didx << 12));

        for (size_t tidx = 0; tidx < 1024; tidx++)
        {
            page_entry_t *entry = &pte[tidx];
            if (!entry->present)
            {
                continue;
            }

            assert(memoryMap[entry->index] > 0, "The subscript is out of bounds.");
            putPage(PAGE(entry->index));
        }

        // 释放页表
        putPage(PAGE(dentry->index));
    }

    // 释放页目录
    freeKpage(task->pde, 1);
}

int32 sysBrk(void *addr)
{
    uint32 brk = (uint32)addr;
    ASSERT_PAGE(brk);

    task_t *task = runningTask();
    assert(task->uid != KERNEL_USER, "The expectation is user mode.");

    assert(KERNEL_MEMORY_SIZE <= brk && brk < USER_STACK_BOTTOM, "Memory overrun.");

    uint32 old_brk = task->brk;

    if (old_brk > brk)
    {
        for (uint32 page = brk; page < old_brk; page += PAGE_SIZE)
        {
            unlinkPage(page);
        }
    }
    else if (IDX(brk - old_brk) > freePages)
    {
        // out of memory
        return -1;
    }

    task->brk = brk;
    return 0;
}

extern "C" void pageFault(
    uint32 vector,
    uint32 edi, uint32 esi, uint32 ebp, uint32 esp,
    uint32 ebx, uint32 edx, uint32 ecx, uint32 eax,
    uint32 gs, uint32 fs, uint32 es, uint32 ds,
    uint32 vector0, uint32 error, uint32 eip, uint32 cs, uint32 eflags)
{
    assert(vector == 0xe, "The interrupt vector number is incorrect.");
    uint32 vaddr = getCr2();

    page_error_code_t *code = (page_error_code_t *)&error;
    task_t *task = runningTask();

    assert(KERNEL_MEMORY_SIZE <= vaddr && vaddr < USER_STACK_TOP, "");

    if (code->present)
    {
        assert(code->write, "");

        page_entry_t *pte = getPte(vaddr, false);
        page_entry_t *entry = &pte[TIDX(vaddr)];

        assert(entry->present, "");
        assert(memoryMap[entry->index] > 0, "Index overshoot.");
        if (memoryMap[entry->index] == 1)
        {
            entry->write = true;
        }
        else
        {
            void *page = (void *)PAGE(IDX(vaddr));
            uint32 paddr = copyPage(page);
            memoryMap[entry->index]--;
            entryInit(entry, IDX(paddr));
            flushTlb(vaddr);
        }
        return;
    }

    if (!code->present && (vaddr < task->brk || vaddr >= USER_STACK_BOTTOM))
    {
        uint32 page = PAGE(IDX(vaddr));
        linkPage(page);
        return;
    }

    panic("page fault!!!");
}