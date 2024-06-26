#include <kernel/ide.h>
#include <io/io.h>
#include <kernel/printk.h>
#include <lib/stdio.h>
#include <kernel/memory.h>
#include <kernel/interrupt.h>
#include <kernel/task.h>
#include <lib/charArray.h>
#include <kernel/assert.h>
#include <kernel/debug.h>
#include <kernel/device.h>
#include <kernel/logk.h>

// IDE 寄存器基址
#define IDE_IOBASE_PRIMARY 0x1F0   // 主通道基地址
#define IDE_IOBASE_SECONDARY 0x170 // 从通道基地址

// IDE 寄存器偏移
#define IDE_DATA 0x0000       // 数据寄存器
#define IDE_ERR 0x0001        // 错误寄存器
#define IDE_FEATURE 0x0001    // 功能寄存器
#define IDE_SECTOR 0x0002     // 扇区数量
#define IDE_LBA_LOW 0x0003    // LBA 低字节
#define IDE_LBA_MID 0x0004    // LBA 中字节
#define IDE_LBA_HIGH 0x0005   // LBA 高字节
#define IDE_HDDEVSEL 0x0006   // 磁盘选择寄存器
#define IDE_STATUS 0x0007     // 状态寄存器
#define IDE_COMMAND 0x0007    // 命令寄存器
#define IDE_ALT_STATUS 0x0206 // 备用状态寄存器
#define IDE_CONTROL 0x0206    // 设备控制寄存器
#define IDE_DEVCTRL 0x0206    // 驱动器地址寄存器

// IDE 命令

#define IDE_CMD_READ 0x20     // 读命令
#define IDE_CMD_WRITE 0x30    // 写命令
#define IDE_CMD_IDENTIFY 0xEC // 识别命令

// IDE 控制器状态寄存器
#define IDE_SR_NULL 0x00 // NULL
#define IDE_SR_ERR 0x01  // Error
#define IDE_SR_IDX 0x02  // Index
#define IDE_SR_CORR 0x04 // Corrected data
#define IDE_SR_DRQ 0x08  // Data request
#define IDE_SR_DSC 0x10  // Drive seek complete
#define IDE_SR_DWF 0x20  // Drive write fault
#define IDE_SR_DRDY 0x40 // Drive ready
#define IDE_SR_BSY 0x80  // Controller busy

// IDE 控制寄存器
#define IDE_CTRL_HD15 0x00 // Use 4 bits for head (not used, was 0x08)
#define IDE_CTRL_SRST 0x04 // Soft reset
#define IDE_CTRL_NIEN 0x02 // Disable interrupts

// IDE 错误寄存器
#define IDE_ER_AMNF 0x01  // Address mark not found
#define IDE_ER_TK0NF 0x02 // Track 0 not found
#define IDE_ER_ABRT 0x04  // Abort
#define IDE_ER_MCR 0x08   // Media change requested
#define IDE_ER_IDNF 0x10  // Sector id not found
#define IDE_ER_MC 0x20    // Media change
#define IDE_ER_UNC 0x40   // Uncorrectable data error
#define IDE_ER_BBK 0x80   // Bad block

#define IDE_LBA_MASTER 0b11100000 // 主盘 LBA
#define IDE_LBA_SLAVE 0b11110000  // 从盘 LBA

// 分区文件系统
// 参考 https://www.win.tue.nl/~aeb/partitions/partition_types-1.html
typedef enum PART_FS
{
    PART_FS_FAT12 = 1,    // FAT12
    PART_FS_EXTENDED = 5, // 扩展分区
    PART_FS_MINIX = 0x80, // minux
    PART_FS_LINUX = 0x83, // linux
}PART_FS;

#pragma pack(1)
typedef struct ide_params_t
{
    uint16 config;                 // 0 General configuration bits
    uint16 cylinders;              // 01 cylinders
    uint16 RESERVED;               // 02
    uint16 heads;                  // 03 heads
    uint16 RESERVED[5 - 3];        // 05
    uint16 sectors;                // 06 sectors per track
    uint16 RESERVED[9 - 6];        // 09
    uint8 serial[20];              // 10 ~ 19 序列号
    uint16 RESERVED[22 - 19];      // 10 ~ 22
    uint8 firmware[8];             // 23 ~ 26 固件版本
    uint8 model[40];               // 27 ~ 46 模型数
    uint8 drqSectors;             // 47 扇区数量
    uint8 RESERVED[3];             // 48
    uint16 capabilities;           // 49 能力
    uint16 RESERVED[59 - 49];      // 50 ~ 59
    uint32 totalLba;              // 60 ~ 61
    uint16 RESERVED;               // 62
    uint16 mdmaMode;              // 63
    uint8 RESERVED;                // 64
    uint8 pioMode;                // 64
    uint16 RESERVED[79 - 64];      // 65 ~ 79 参见 ATA specification
    uint16 majorVersion;          // 80 主版本
    uint16 minorVersion;          // 81 副版本
    uint16 commmandSets[87 - 81]; // 82 ~ 87 支持的命令集
    uint16 RESERVED[118 - 87];     // 88 ~ 118
    uint16 supportSettings;       // 119
    uint16 enableSettings;        // 120
    uint16 RESERVED[221 - 120];    // 221
    uint16 transportMajor;        // 222
    uint16 transportMinor;        // 223
    uint16 RESERVED[254 - 223];    // 254
    uint16 integrity;              // 校验和
}ide_params_t;
#pragma pack()

ide_ctrl_t controllers[IDE_CTRL_NR];


static void ideHandler(int32 vector)
{
    sendEoi(vector); // 向中断控制器发送中断处理结束信号

    // 得到中断向量对应的控制器
    ide_ctrl_t *ctrl = &controllers[vector - IRQ_HARDDISK - 0x20];

    // 读取常规状态寄存器，表示中断处理结束
    uint8 state = inByte(ctrl->iobase + IDE_STATUS);
    LOGK("harddisk interrupt vector %d state 0x%x\n", vector, state);
    if (ctrl->waiter)
    {
        // 如果有进程阻塞，则取消阻塞
        taskUnblock(ctrl->waiter);
        ctrl->waiter = nullptr;
    }
}

static uint32 ideError(ide_ctrl_t *ctrl)
{
    uint8 error = inByte(ctrl->iobase + IDE_ERR);
    if (error & IDE_ER_BBK)
        LOGK("bad block");
    if (error & IDE_ER_UNC)
        LOGK("uncorrectable data");
    if (error & IDE_ER_MC)
        LOGK("media change");
    if (error & IDE_ER_IDNF)
        LOGK("id not found");
    if (error & IDE_ER_MCR)
        LOGK("media change requested");
    if (error & IDE_ER_ABRT)
        LOGK("abort");
    if (error & IDE_ER_TK0NF)
        LOGK("track 0 not found");
    if (error & IDE_ER_AMNF)
        LOGK("address mark not found");
    return error;
}

static uint32 ideBusyWait(ide_ctrl_t *ctrl, uint8 mask)
{
    while (true)
    {
        // 从备用状态寄存器中读状态
        uint8 state = inByte(ctrl->iobase + IDE_ALT_STATUS);
        if (state & IDE_SR_ERR) // 有错误
        {
            ideError(ctrl);
        }
        if (state & IDE_SR_BSY) // 驱动器忙
        {
            continue;
        }
        if ((state & mask) == mask) // 等待的状态完成
            return 0;
    }
}

// 重置硬盘控制器
static void ideResetController(ide_ctrl_t *ctrl)
{
    outByte(ctrl->iobase + IDE_CONTROL, IDE_CTRL_SRST);
    ideBusyWait(ctrl, IDE_SR_NULL);
    outByte(ctrl->iobase + IDE_CONTROL, ctrl->control);
    ideBusyWait(ctrl, IDE_SR_NULL);
}

// 选择磁盘
static void ideSelectDrive(ide_disk_t *disk)
{
    outByte(disk->ctrl->iobase + IDE_HDDEVSEL, disk->selector);
    disk->ctrl->active = disk;
}

// 选择扇区
static void ideSelectSector(ide_disk_t *disk, uint32 lba, uint8 count)
{
    // 输出功能，可省略
    outByte(disk->ctrl->iobase + IDE_FEATURE, 0);

    // 读写扇区数量
    outByte(disk->ctrl->iobase + IDE_SECTOR, count);

    // LBA 低字节
    outByte(disk->ctrl->iobase + IDE_LBA_LOW, lba & 0xff);
    // LBA 中字节
    outByte(disk->ctrl->iobase + IDE_LBA_MID, (lba >> 8) & 0xff);
    // LBA 高字节
    outByte(disk->ctrl->iobase + IDE_LBA_HIGH, (lba >> 16) & 0xff);

    // LBA 最高四位 + 磁盘选择
    outByte(disk->ctrl->iobase + IDE_HDDEVSEL, ((lba >> 24) & 0xf) | disk->selector);
    disk->ctrl->active = disk;
}

// 从磁盘读取一个扇区到 buf
static void idePioReadSector(ide_disk_t *disk, uint16 *buf)
{
    for (size_t i = 0; i < (SECTOR_SIZE / 2); i++)
    {
        buf[i] = inWord(disk->ctrl->iobase + IDE_DATA);
    }
}

// 从 buf 写入一个扇区到磁盘
static void idePioWriteSector(ide_disk_t *disk, uint16 *buf)
{
    for (size_t i = 0; i < (SECTOR_SIZE / 2); i++)
    {
        outWord(disk->ctrl->iobase + IDE_DATA, buf[i]);
    }
}

// 磁盘控制
int idePioIoctl(ide_disk_t *disk, int cmd, void *args, int flags)
{
    switch (cmd)
    {
    case DEV_CMD_SECTOR_START:
        return 0;
    case DEV_CMD_SECTOR_COUNT:
        return disk->totalLba;
    default:
        panic("device command %d can't recognize!!!", cmd);
        return -1;
    }
}

// PIO 方式读取磁盘
int idePioRead(ide_disk_t *disk, void *buf, size_t _count, idx_t lba)
{
    uint8 count = (uint8)_count;
    assert(count > 0, "");
    assert(!getInterruptState(), ""); // 异步方式，调用该函数时不许中断

    ide_ctrl_t *ctrl = disk->ctrl;

    lockAcquire(&ctrl->lock);

    // 选择磁盘
    ideSelectDrive(disk);

    // 等待就绪
    ideBusyWait(ctrl, IDE_SR_DRDY);

    // 选择扇区
    ideSelectSector(disk, lba, count);

    // 发送读命令
    outByte(ctrl->iobase + IDE_COMMAND, IDE_CMD_READ);

    for (size_t i = 0; i < count; i++)
    {
        task_t *task = runningTask();
        if (task->state == TASK_RUNNING) // 系统初始化时，不能使用异步方式
        {
            // 阻塞自己等待中断的到来，等待磁盘准备数据
            ctrl->waiter = task;
            taskBlock(task, nullptr, TASK_BLOCKED);
        }

        ideBusyWait(ctrl, IDE_SR_DRQ);
        uint32 offset = ((uint32)buf + i * SECTOR_SIZE);
        idePioReadSector(disk, (uint16 *)offset);
    }

    lockRelease(&ctrl->lock);
    return 0;
}

// PIO 方式写磁盘
int idePioWrite(ide_disk_t *disk, void *buf, size_t _count, idx_t lba)
{
    uint8 count = (uint8)(_count);
    assert(count > 0, "");
    assert(!getInterruptState(), ""); // 异步方式，调用该函数时不许中断

    ide_ctrl_t *ctrl = disk->ctrl;

    lockAcquire(&ctrl->lock);

    // 选择磁盘
    ideSelectDrive(disk);

    // 等待就绪
    ideBusyWait(ctrl, IDE_SR_DRDY);

    // 选择扇区
    ideSelectSector(disk, lba, count);

    // 发送写命令
    outByte(ctrl->iobase + IDE_COMMAND, IDE_CMD_WRITE);

    for (size_t i = 0; i < count; i++)
    {
        uint32 offset = ((uint32)buf + i * SECTOR_SIZE);
        idePioWriteSector(disk, (uint16 *)offset);

        task_t *task = runningTask();
        if (task->state == TASK_RUNNING) // 系统初始化时，不能使用异步方式
        {
            // 阻塞自己等待磁盘写数据完成
            ctrl->waiter = task;
            taskBlock(task, nullptr, TASK_BLOCKED);
        }
        ideBusyWait(ctrl, IDE_SR_NULL);
    }

    lockRelease(&ctrl->lock);
    return 0;
}

// 分区控制
int idePioPartIoctl(ide_part_t *part, int cmd, void *args, int flags)
{
    switch (cmd)
    {
    case DEV_CMD_SECTOR_START:
        return part->start;
    case DEV_CMD_SECTOR_COUNT:
        return part->count;
    default:
        panic("device command %d can't recognize!!!", cmd);
        return -1;
    }
}

// 读分区
int idePioPartRead(ide_part_t *part, void *buf, size_t count, idx_t lba)
{
    return idePioRead(part->disk, buf, count, part->start + (idx_t)lba);
}

// 写分区
int idePioPartWrite(ide_part_t *part, void *buf, size_t count, idx_t lba)
{
    return idePioWrite(part->disk, buf, count, part->start + (idx_t)lba);
}


static void ideSwapPairs(char *buf, uint32 len)
{
    for (size_t i = 0; i < len; i += 2)
    {
        char ch = buf[i];
        buf[i] = buf[i + 1];
        buf[i + 1] = ch;
    }
    buf[len - 1] = '\0';
}

static uint32 ideIdentify(ide_disk_t *disk, uint16 *buf)
{
    LOGK("identifing disk %s...", disk->name);
    lockAcquire(&disk->ctrl->lock);
    ideSelectDrive(disk);

    // ide_select_sector(disk, 0, 0);

    outByte(disk->ctrl->iobase + IDE_COMMAND, IDE_CMD_IDENTIFY);

    ideBusyWait(disk->ctrl, IDE_SR_NULL);

    ide_params_t *params = (ide_params_t *)buf;

    idePioReadSector(disk, buf);

    LOGK("disk %s total lba %d", disk->name, params->totalLba);

    uint32 ret = EOF;
    if (params->totalLba == 0)
    {
        lockRelease(&disk->ctrl->lock);
        return ret;
    }

    ideSwapPairs((char*)params->serial, sizeof(params->serial));
    LOGK("disk %s serial number %s", disk->name, params->serial);

    ideSwapPairs((char*)params->firmware, sizeof(params->firmware));
    LOGK("disk %s firmware version %s", disk->name, params->firmware);

    ideSwapPairs((char*)params->model, sizeof(params->model));
    LOGK("disk %s model number %s", disk->name, params->model);

    disk->totalLba = params->totalLba;
    disk->cylinders = params->cylinders;
    disk->heads = params->heads;
    disk->sectors = params->sectors;
    ret = 0;

    lockRelease(&disk->ctrl->lock);
    return ret;
}

static void idePartInit(ide_disk_t *disk, uint16 *buf)
{
    // 磁盘不可用
    if (!disk->totalLba)
        return;

    // 读取主引导扇区
    idePioRead(disk, buf, 1, 0);

    // 初始化主引导扇区
    boot_sector_t *boot = (boot_sector_t *)buf;

    for (size_t i = 0; i < IDE_PART_NR; i++)
    {
        part_entry_t *entry = &boot->entry[i];
        ide_part_t *part = &disk->parts[i];
        if (!entry->count)
            continue;

        sprintf(part->name, "%s%d", disk->name, i + 1);

        LOGK("part %s ", part->name);
        LOGK("    bootable %d", entry->bootable);
        LOGK("    start %d", entry->start);
        LOGK("    count %d", entry->count);
        LOGK("    system 0x%x", entry->system);

        part->disk = disk;
        part->count = entry->count;
        part->system = entry->system;
        part->start = entry->start;

        if (entry->system == PART_FS_EXTENDED)
        {
            LOGK("Unsupported extended partition!!!\n");

            boot_sector_t *eboot = (boot_sector_t *)(buf + SECTOR_SIZE);
            idePioRead(disk, (void *)eboot, 1, entry->start);

            for (size_t j = 0; j < IDE_PART_NR; j++)
            {
                part_entry_t *eentry = &eboot->entry[j];
                if (!eentry->count)
                    continue;
                LOGK("part %d extend %d", i, j);
                LOGK("    bootable %d", eentry->bootable);
                LOGK("    start %d", eentry->start);
                LOGK("    count %d", eentry->count);
                LOGK("    system 0x%x", eentry->system);
            }
        }
    }
}

static void ideCtrlInit()
{
    uint16 *buf = (uint16 *)allocKpage(1);
    for (size_t cidx = 0; cidx < IDE_CTRL_NR; cidx++)
    {
        ide_ctrl_t *ctrl = &controllers[cidx];
        sprintf(ctrl->name, "ide%u", cidx);
        lockInit(&ctrl->lock);
        ctrl->active = nullptr;
        ctrl->waiter = nullptr;

        if (cidx) // 从通道
        {
            ctrl->iobase = IDE_IOBASE_SECONDARY;
        }
        else // 主通道
        {
            ctrl->iobase = IDE_IOBASE_PRIMARY;
        }

        ctrl->control = inByte(ctrl->iobase + IDE_CONTROL);

        for (size_t didx = 0; didx < IDE_DISK_NR; didx++)
        {
            ide_disk_t *disk = &ctrl->disks[didx];
            sprintf(disk->name, "hd%c", 'a' + cidx * 2 + didx);
            disk->ctrl = ctrl;
            if (didx) // 从盘
            {
                disk->master = false;
                disk->selector = IDE_LBA_SLAVE;
            }
            else // 主盘
            {
                disk->master = true;
                disk->selector = IDE_LBA_MASTER;
            }
            ideIdentify(disk, buf);
            idePartInit(disk, buf);
        }
    }
    freeKpage((uint32)buf, 1);
}

static void ideInstall()
{
    for (size_t cidx = 0; cidx < IDE_CTRL_NR; cidx++)
    {
        ide_ctrl_t *ctrl = &controllers[cidx];
        for (size_t didx = 0; didx < IDE_DISK_NR; didx++)
        {
            ide_disk_t *disk = &ctrl->disks[didx];
            if (!disk->totalLba)
                continue;
            dev_t dev = deviceInstall(
                DEV_BLOCK, DEV_IDE_DISK, disk, disk->name, 0,
                (void*)idePioIoctl, (void*)idePioRead, (void*)idePioWrite);
            for (size_t i = 0; i < IDE_PART_NR; i++)
            {
                ide_part_t *part = &disk->parts[i];
                if (!part->count)
                    continue;
                deviceInstall(
                    DEV_BLOCK, DEV_IDE_PART, part, part->name, dev,
                    (void*)idePioPartIoctl, (void*)idePioPartRead, (void*)idePioPartWrite);
            }
        }
    }
}

void ideInit()
{
    LOGK("ide init...");
    ideCtrlInit();
    ideInstall(); // 安装设备
    // 注册硬盘中断，并打开屏蔽字
    setInterruptHandler(IRQ_HARDDISK, ideHandler);
    setInterruptHandler(IRQ_HARDDISK2, ideHandler);
    setInterruptMask(IRQ_HARDDISK, true);
    setInterruptMask(IRQ_HARDDISK2, true);
    setInterruptMask(IRQ_CASCADE, true);
}