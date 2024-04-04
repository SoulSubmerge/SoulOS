#include <kernel/buffer.h>
#include <kernel/device.h>
#include <kernel/assert.h>
#include <lib/charArray.h>
#include <kernel/debug.h>
#include <kernel/logk.h>
#include <kernel/logk.h>
#include <fs/fs.h>

#define SUPER_NR 16

static super_block_t superTable[SUPER_NR]; // 超级块表
static super_block_t *root;                 // 根文件系统超级块

// 从超级块表中查找一个空闲块
static super_block_t *getFreeSuper()
{
    for (size_t i = 0; i < SUPER_NR; i++)
    {
        super_block_t *sb = &superTable[i];
        if (sb->dev == EOF)
        {
            return sb;
        }
    }
    panic("no more super block!!!");
    return nullptr;
}

// 获得设备 dev 的超级块
super_block_t *getSuper(dev_t dev)
{
    for (size_t i = 0; i < SUPER_NR; i++)
    {
        super_block_t *sb = &superTable[i];
        if (sb->dev == dev)
        {
            return sb;
        }
    }
    return nullptr;
}

// 读设备 dev 的超级块
super_block_t *readSuper(dev_t dev)
{
    super_block_t *sb = getSuper(dev);
    if (sb)
    {
        return sb;
    }

    LOGK("Reading super block of device %d\n", dev);


    // 获得空闲超级块
    sb = getFreeSuper();

    // 读取超级块
    buffer_t *buf = bread(dev, 1);

    sb->buf = buf;
    sb->desc = (super_desc_t *)buf->data;
    sb->dev = dev;

    assert(sb->desc->magic == MINIX1_MAGIC, "");

    memset(sb->imaps, 0, sizeof(sb->imaps));
    memset(sb->zmaps, 0, sizeof(sb->zmaps));

    // 读取 inode 位图
    int idx = 2; // 块位图从第 2 块开始，第 0 块 引导块，第 1 块 超级块

    for (int i = 0; i < sb->desc->imapBlocks; i++)
    {
        assert(i < IMAP_NR, "");
        if ((sb->imaps[i] = bread(dev, idx)))
            idx++;
        else
            break;
    }


    // 读取块位图
    for (int i = 0; i < sb->desc->zmapBlocks; i++)
    {
        assert(i < ZMAP_NR, "");
        if ((sb->zmaps[i] = bread(dev, idx)))
            idx++;
        else
            break;
    }

    return sb;
}

// 挂载根文件系统
static void mountRoot()
{
    LOGK("Mount root file system...\n");
    // 假设主硬盘第一个分区是根文件系统
    device_t *device = deviceFind(DEV_IDE_PART, 0);
    assert(device, "");

    // 读根文件系统超级块
    root = readSuper(device->dev);

     // 初始化根目录 inode
    root->iroot = iget(device->dev, 1);  // 获得根目录 inode
    root->imount = iget(device->dev, 1); // 根目录挂载 inode
}

void superInit()
{
    for (size_t i = 0; i < SUPER_NR; i++)
    {
        super_block_t *sb = &superTable[i];
        sb->dev = EOF;
        sb->desc = nullptr;
        sb->buf = nullptr;
        sb->iroot = nullptr;
        sb->imount = nullptr;
        listInit(&sb->inodeList);
    }
    mountRoot();
    LOGK("SuperInit Finish!!!\n");
}