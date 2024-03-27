#ifndef SOUL_IDE_H
#define SOUL_IDE_H

#include <types/types.h>
#include <kernel/mutex.h>

#define SECTOR_SIZE 512 // 扇区大小

#define IDE_CTRL_NR 2 // 控制器数量，固定为 2
#define IDE_DISK_NR 2 // 每个控制器可挂磁盘数量，固定为 2

// IDE 磁盘
typedef struct ide_disk_t
{
    char name[8];            // 磁盘名称
    struct ide_ctrl_t *ctrl; // 控制器指针
    uint8 selector;             // 磁盘选择
    bool master;             // 主盘
} ide_disk_t;

// IDE 控制器
typedef struct ide_ctrl_t
{
    char name[8];                   // 控制器名称
    lock_t lock;                    // 控制器锁
    uint16 iobase;                  // IO 寄存器基址
    ide_disk_t disks[IDE_DISK_NR];  // 磁盘
    ide_disk_t *active;             // 当前选择的磁盘
    task_t *waiter;                 // 等待控制器的进程
} ide_ctrl_t;

int idePioRead(ide_disk_t *disk, void *buf, uint8 count, idx_t lba);
int idePioWrite(ide_disk_t *disk, void *buf, uint8 count, idx_t lba);

#endif