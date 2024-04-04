#include <lib/syscall.h>
#include <kernel/assert.h>
#include <kernel/debug.h>
#include <kernel/arena.h>
#include <lib/charArray.h>
#include <lib/stdlib.h>
#include <kernel/logk.h>
#include <kernel/buffer.h>
#include <fs/fs.h>

#define INODE_NR 64

static inode_t inodeTable[INODE_NR];

// 申请一个 inode
static inode_t *getFreeInode()
{
    for (size_t i = 0; i < INODE_NR; i++)
    {
        inode_t *inode = &inodeTable[i];
        if (inode->dev == EOF)
        {
            return inode;
        }
    }
    panic("no more inode!!!");
    return nullptr;
}

// 释放一个 inode
static void putFreeInode(inode_t *inode)
{
    assert(inode != inodeTable, "");
    assert(inode->count == 0, "");
    inode->dev = EOF;
}

// 获取根 inode
inode_t *getRootInode()
{
    return inodeTable;
}

// 计算 inode nr 对应的块号
static inline idx_t inodeBlock(super_block_t *sb, idx_t nr)
{
    // inode 编号 从 1 开始
    return 2 + sb->desc->imapBlocks + sb->desc->zmapBlocks + (nr - 1) / BLOCK_INODES;
}

// 从已有 inode 中查找编号为 nr 的 inode
static inode_t *findInode(dev_t dev, idx_t nr)
{
    super_block_t *sb = getSuper(dev);
    assert(sb, "");
    list_t *list = &sb->inodeList;

    for (list_node_t *node = list->head.next; node != &list->tail; node = node->next)
    {
        inode_t *inode = ELEMENT_ENTRY(inode_t, node, node);
        if (inode->nr == nr)
        {
            return inode;
        }
    }
    return nullptr;
}

// 获得设备 dev 的 nr inode
inode_t *iget(dev_t dev, idx_t nr)
{
    inode_t *inode = findInode(dev, nr);
    if (inode)
    {
        inode->count++;
        inode->atime = time();

        return inode;
    }

    super_block_t *sb = getSuper(dev);
    assert(sb, "");

    assert(nr <= sb->desc->inodes, "");

    inode = getFreeInode();
    inode->dev = dev;
    inode->nr = nr;
    inode->count = 1;

    // 加入超级块 inode 链表
    listPush(&sb->inodeList, &inode->node);

    idx_t block = inodeBlock(sb, inode->nr);
    buffer_t *buf = bread(inode->dev, block);

    inode->buf = buf;

    // 将缓冲视为一个 inode 描述符数组，获取对应的指针；
    inode->desc = &((inode_desc_t *)buf->data)[(inode->nr - 1) % BLOCK_INODES];

    inode->ctime = inode->desc->mtime;
    inode->atime = time();

    return inode;
}

// 释放 inode
void iput(inode_t *inode)
{
    if (!inode)
        return;

    inode->count--;

    if (inode->count)
    {
        return;
    }

    // 释放 inode 对应的缓冲
    brelse(inode->buf);

    // 从超级块链表中移除
    listRemove(&inode->node);

    // 释放 inode 内存
    putFreeInode(inode);
}

void inodeInit()
{
    for (size_t i = 0; i < INODE_NR; i++)
    {
        inode_t *inode = &inodeTable[i];
        inode->dev = EOF;
    }
}