#include <kernel/arena.h>
#include <kernel/memory.h>
#include <lib/charArray.h>
#include <lib/stdlib.h>
#include <kernel/assert.h>
#include <kernel/kernel.h>


extern uint32 freePages;
static arena_descriptor_t descriptors[DESC_COUNT];

// arena 初始化
void arenaInit()
{
    uint32 blockSize = 16;
    for (size_t i = 0; i < DESC_COUNT; i++)
    {
        arena_descriptor_t *desc = &descriptors[i];
        desc->blockSize = blockSize;
        desc->totalBlock = (PAGE_SIZE - sizeof(arena_t)) / blockSize;
        listInit(&desc->freeList);
        blockSize <<= 1; // block *= 2;
    }
}

// 获得 arena 第 idx 块内存指针
static void *getArenaBlock(arena_t *arena, uint32 idx)
{
    assert(arena->desc->totalBlock > idx, "The subscript is out of bounds.");
    void *addr = (void *)(arena + 1);
    uint32 gap = idx * arena->desc->blockSize;
    return (void*)((uint32)addr + gap);
}

static arena_t *getBlockArena(block_t *block)
{
    return (arena_t *)((uint32)block & 0xfffff000);
}

void *kmalloc(size_t size)
{
    arena_descriptor_t *desc = nullptr;
    arena_t *arena;
    block_t *block;
    char *addr;

    if (size > 1024)
    {
        uint32 asize = size + sizeof(arena_t);
        uint32 count = divRoundUp(asize, PAGE_SIZE);

        arena = (arena_t *)allocKpage(count);
        memset(arena, 0, count * PAGE_SIZE);
        arena->large = true;
        arena->count = count;
        arena->desc = nullptr;
        arena->magic = SOUL_MAGIC;

        addr = (char *)((uint32)arena + sizeof(arena_t));
        return addr;
    }

    for (size_t i = 0; i < DESC_COUNT; i++)
    {
        desc = &descriptors[i];
        if (desc->blockSize >= size)
            break;
    }

    assert(desc != nullptr, "illegal address.");

    if (listEmpty(&desc->freeList))
    {
        arena = (arena_t *)allocKpage(1);
        memset(arena, 0, PAGE_SIZE);

        arena->desc = desc;
        arena->large = false;
        arena->count = desc->totalBlock;
        arena->magic = SOUL_MAGIC;

        for (size_t i = 0; i < desc->totalBlock; i++)
        {
            block = (block_t*)getArenaBlock(arena, i);
            assert(!listSearch(&arena->desc->freeList, block), "Ineffectual fast.");
            listPush(&arena->desc->freeList, block);
            assert(listSearch(&arena->desc->freeList, block), "Ineffectual fast.");
        }
    }

    block = listPop(&desc->freeList);

    arena = getBlockArena(block);
    assert(arena->magic == SOUL_MAGIC && !arena->large, "Memory error occurred.");
    // memset(block, 0, desc->blockSize);
    arena->count--;
    return block;
}

void kfree(void *ptr)
{
    assert(ptr, "Access illegal address.");

    block_t *block = (block_t *)ptr;
    arena_t *arena = getBlockArena(block);

    assert(arena->large == 1 || arena->large == 0, "Invalid identification.");
    assert(arena->magic == SOUL_MAGIC, "Memory corruption.");

    if (arena->large)
    {
        freeKpage((uint32)arena, arena->count);
        return;
    }

    listPush(&arena->desc->freeList, block);
    arena->count++;

    if (arena->count == arena->desc->totalBlock)
    {
        for (size_t i = 0; i < arena->desc->totalBlock; i++)
        {
            block = (block_t*)getArenaBlock(arena, i);
            assert(listSearch(&arena->desc->freeList, block), "Ineffectual fast.");
            listRemove(block);
            assert(!listSearch(&arena->desc->freeList, block), "Ineffectual fast.");
        }
        freeKpage((uint32)arena, 1);
    }
}