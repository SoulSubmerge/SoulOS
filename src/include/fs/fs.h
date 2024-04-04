#ifndef SOUL_FS_H
#define SOUL_FS_H

#include <types/types.h>
#include <lib/list.h>

#define BLOCK_SIZE 1024 // 块大小
#define SECTOR_SIZE 512 // 扇区大小

#define MINIX1_MAGIC 0x137F // 文件系统魔数
#define NAME_LEN 14        // 文件名长度

#define IMAP_NR 8 // inode 位图块，最大值
#define ZMAP_NR 8 // 块位图块，最大值

#define BLOCK_BITS (BLOCK_SIZE * 8)                      // 块位图大小
#define BLOCK_INODES (BLOCK_SIZE / sizeof(inode_desc_t)) // 块 inode 数量
#define BLOCK_DENTRIES (BLOCK_SIZE / sizeof(dentry_t))   // 块 dentry 数量
#define BLOCK_INDEXES (BLOCK_SIZE / sizeof(uint16))         // 块索引数量

#define DIRECT_BLOCK (7)                                               // 直接块数量
#define INDIRECT1_BLOCK BLOCK_INDEXES                                  // 一级间接块数量
#define INDIRECT2_BLOCK (INDIRECT1_BLOCK * INDIRECT1_BLOCK)            // 二级间接块数量
#define TOTAL_BLOCK (DIRECT_BLOCK + INDIRECT1_BLOCK + INDIRECT2_BLOCK) // 全部块数量

typedef struct inode_desc_t
{
    uint16 mode;    // 文件类型和属性(rwx 位)
    uint16 uid;     // 用户id（文件拥有者标识符）
    uint32 size;    // 文件大小（字节数）
    uint32 mtime;   // 修改时间戳 这个时间戳应该用 UTC 时间，不然有瑕疵
    uint8 gid;      // 组id(文件拥有者所在的组)
    uint8 nlinks;   // 链接数（多少个文件目录项指向该i 节点）
    uint16 zone[9]; // 直接 (0-6)、间接(7)或双重间接 (8) 逻辑块号
} inode_desc_t;

struct buffer_t;

typedef struct inode_t
{
    inode_desc_t *desc;   // inode 描述符
    buffer_t *buf; // inode 描述符对应 buffer
    dev_t dev;            // 设备号
    idx_t nr;             // i 节点号
    uint32 count;            // 引用计数
    time_t atime;         // 访问时间
    time_t ctime;         // 创建时间
    list_node_t node;     // 链表结点
    dev_t mount;          // 安装设备
}inode_t;

typedef struct super_desc_t
{
    uint16 inodes;        // 节点数
    uint16 zones;         // 逻辑块数
    uint16 imapBlocks;   // i 节点位图所占用的数据块数
    uint16 zmapBlocks;   // 逻辑块位图所占用的数据块数
    uint16 firstdatazone; // 第一个数据逻辑块号
    uint16 logZoneSize; // log2(每逻辑块数据块数)
    uint32 maxSize;      // 文件最大长度
    uint16 magic;         // 文件系统魔数
} super_desc_t;

typedef struct super_block_t
{
    super_desc_t *desc;              // 超级块描述符
    buffer_t *buf;            // 超级快描述符 buffer
    buffer_t *imaps[IMAP_NR]; // inode 位图缓冲
    buffer_t *zmaps[ZMAP_NR]; // 块位图缓冲
    dev_t dev;                       // 设备号
    list_t inodeList;               // 使用中 inode 链表
    inode_t *iroot;                  // 根目录 inode
    inode_t *imount;                 // 安装到的 inode
} super_block_t;

// 文件目录项结构
typedef struct dentry_t
{
    uint16 nr;              // i 节点
    char name[NAME_LEN]; // 文件名
} dentry_t;

super_block_t *getSuper(dev_t dev);  // 获得 dev 对应的超级块
super_block_t *readSuper(dev_t dev); // 读取 dev 对应的超级块

idx_t balloc(dev_t dev);          // 分配一个文件块
void bfree(dev_t dev, idx_t idx); // 释放一个文件块
idx_t ialloc(dev_t dev);          // 分配一个文件系统 inode
void ifree(dev_t dev, idx_t idx); // 释放一个文件系统 inode

// 获取 inode 第 block 块的索引值
// 如果不存在 且 create 为 true，则创建
idx_t bmap(inode_t *inode, idx_t block, bool create);

inode_t *getRootInode();            // 获取根目录 inode
inode_t *iget(dev_t dev, idx_t nr); // 获得设备 dev 的 nr inode
void iput(inode_t *inode);          // 释放 inode

#endif