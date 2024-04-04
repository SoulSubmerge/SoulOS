#include <kernel/buffer.h>
#include <kernel/memory.h>
#include <kernel/debug.h>
#include <kernel/assert.h>
#include <kernel/device.h>
#include <lib/charArray.h>
#include <kernel/task.h>
#include <kernel/logk.h>

#define HASH_COUNT 31 // 应该是个素数

static buffer_t *bufferStart = (buffer_t *)KERNEL_BUFFER_MEM;
static uint32 bufferCount = 0;

// 记录当前 buffer_t 结构体位置
static buffer_t *bufferPtr = (buffer_t *)KERNEL_BUFFER_MEM;

// 记录当前数据缓冲区位置
static void *bufferData = (void *)(KERNEL_BUFFER_MEM + KERNEL_BUFFER_SIZE - BLOCK_SIZE);

static list_t freeList;              // 缓存链表，被释放的块
static list_t waitList;              // 等待进程链表
static list_t hashTable[HASH_COUNT]; // 缓存哈希表

// 哈希函数
uint32 hash(dev_t dev, idx_t block)
{
    return (dev ^ block) % HASH_COUNT;
}

// 从哈希表中查找 buffer
static buffer_t *getFromHashTable(dev_t dev, idx_t block)
{
    uint32 idx = hash(dev, block);
    list_t *list = &hashTable[idx];
    buffer_t *bf = nullptr;

    for (list_node_t *node = list->head.next; node != &list->tail; node = node->next)
    {
        buffer_t *ptr = ELEMENT_ENTRY(buffer_t, hnode, node);
        if (ptr->dev == dev && ptr->block == block)
        {
            bf = ptr;
            break;
        }
    }

    if (!bf)
    {
        return nullptr;
    }

    // 如果 bf 在空闲列表中，则移除
    if (listSearch(&freeList, &bf->rnode))
    {
        listRemove(&bf->rnode);
    }

    return bf;
}

// 将 bf 放入哈希表
static void hashLocate(buffer_t *bf)
{
    uint32 idx = hash(bf->dev, bf->block);
    list_t *list = &hashTable[idx];
    assert(!listSearch(list, &bf->hnode), "");
    listPush(list, &bf->hnode);
}

// 将 bf 从哈希表中移除
static void hashRemove(buffer_t *bf)
{
    uint32 idx = hash(bf->dev, bf->block);
    list_t *list = &hashTable[idx];
    assert(listSearch(list, &bf->hnode), "");
    listRemove(&bf->hnode);
}

// 直接初始化过慢，按需取用
static buffer_t *getNewBuffer()
{
    buffer_t *bf = nullptr;

    if ((uint32)bufferPtr + sizeof(buffer_t) < (uint32)bufferData)
    {
        bf = bufferPtr;
        bf->data = (char*)bufferData;
        bf->dev = EOF;
        bf->block = 0;
        bf->count = 0;
        bf->dirty = false;
        bf->valid = false;
        lockInit(&bf->lock);
        bufferCount++;
        bufferPtr++;
        bufferData = (void*)((uint32)(bufferData) + BLOCK_SIZE);
        // LOGK("buffer count %d\n", bufferCount);
    }

    return bf;
}

// 获得空闲的 buffer
static buffer_t *getFreeBuffer()
{
    buffer_t *bf = nullptr;
    while (true)
    {
        // 如果内存够，直接获得缓存
        bf = getNewBuffer();
        if (bf)
        {
            return bf;
        }
        // 否则，从空闲列表中取得
        if (!listEmpty(&freeList))
        {
            // 取最远未访问过的块
            bf = ELEMENT_ENTRY(buffer_t, rnode, listPopback(&freeList));
            hashRemove(bf);
            bf->valid = false;
            return bf;
        }
        // 等待某个缓冲释放
        taskBlock(runningTask(), &waitList, TASK_BLOCKED);
    }
}

// 获得设备 dev，第 block 对应的缓冲
buffer_t *getblk(dev_t dev, idx_t block)
{
    buffer_t *bf = getFromHashTable(dev, block);
    if (bf)
    {
        assert(bf->valid, "");
        return bf;
    }

    bf = getFreeBuffer();
    assert(bf->count == 0, "");
    assert(bf->dirty == 0, "");

    bf->count = 1;
    bf->dev = dev;
    bf->block = block;
    hashLocate(bf);
    return bf;
}

// 读取 dev 的 block 块
buffer_t *bread(dev_t dev, idx_t block)
{
    buffer_t *bf = getblk(dev, block);
    assert(bf != nullptr, "");
    if (bf->valid)
    {
        bf->count++;
        return bf;
    }

    deviceRequest(bf->dev, bf->data, BLOCK_SECS, bf->block * BLOCK_SECS, 0, REQ_READ);

    bf->dirty = false;
    bf->valid = true;
    return bf;
}

// 写缓冲
void bwrite(buffer_t *bf)
{
    assert(bf, "");
    if (!bf->dirty)
        return;
    deviceRequest(bf->dev, bf->data, BLOCK_SECS, bf->block * BLOCK_SECS, 0, REQ_WRITE);
    bf->dirty = false;
    bf->valid = true;
}

// 释放缓冲
void brelse(buffer_t *bf)
{
    if (!bf)
        return;

    if (bf->dirty)
    {
        bwrite(bf); // todo need write?
    }

    bf->count--;
    assert(bf->count >= 0, "");
    if (bf->count) // 还有人用，直接返回
        return;
    assert(!bf->rnode.next, "");
    assert(!bf->rnode.prev, "");
    listPush(&freeList, &bf->rnode);

    if (!listEmpty(&waitList))
    {
        task_t *task = ELEMENT_ENTRY(task_t, node, listPopback(&waitList));
        taskUnblock(task);
    }
}

void bufferInit()
{
    LOGK("buffer_t size is %d\n", sizeof(buffer_t));

    // 初始化空闲链表
    listInit(&freeList);
    // 初始化等待进程链表
    listInit(&waitList);

    // 初始化哈希表
    for (size_t i = 0; i < HASH_COUNT; i++)
    {
        listInit(&hashTable[i]);
    }
}