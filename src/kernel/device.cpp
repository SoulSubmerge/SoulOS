#include <kernel/device.h>
#include <lib/charArray.h>
#include <kernel/task.h>
#include <kernel/assert.h>
#include <kernel/debug.h>
#include <kernel/arena.h>
#include <kernel/logk.h>
#include <kernel/kernel.h>
#include <kernel/logk.h>

#define DEVICE_NR 64 // 设备数量

static device_t devices[DEVICE_NR]; // 设备数组

// 获取空设备
static device_t *getNullDevice()
{
    for (size_t i = 1; i < DEVICE_NR; i++)
    {
        device_t *device = &devices[i];
        if (device->type == DEV_NULL)
            return device;
    }
    panic("no more devices!!!");
    return nullptr;
}

int deviceIoctl(dev_t dev, int cmd, void *args, int flags)
{
    device_t *device = deviceGet(dev);
    if (device->ioctl)
    {
        return device->ioctl(device->ptr, cmd, args, flags);
    }
    LOGK("ioctl of device %d not implemented!!!\n", dev);
    return EOF;
}

int deviceRead(dev_t dev, void *buf, size_t count, idx_t idx, int flags)
{
    device_t *device = deviceGet(dev);
    if (device->read)
    {
        return device->read(device->ptr, buf, (uint8)count, (idx_t)idx, (int)flags);
    }
    LOGK("read of device %d not implemented!!!\n", dev);
    return EOF;
}

int deviceWrite(dev_t dev, void *buf, size_t count, idx_t idx, int flags)
{
    device_t *device = deviceGet(dev);
    if (device->write)
    {
        return device->write(device->ptr, buf,  count, (idx_t)idx, (int)flags);
    }
    LOGK("write of device %d not implemented!!!\n", dev);
    return EOF;
}

// 安装设备
dev_t deviceInstall(
    int type, int subtype,
    void *ptr, const char *name, dev_t parent,
    void *ioctl, void *read, void *write)
{
    device_t *device = getNullDevice();
    device->ptr = ptr;
    device->parent = parent;
    device->type = type;
    device->subtype = subtype;
    strncpy(device->name, name, NAMELEN);
    device->ioctl = (ioctlFnPtr)ioctl;
    device->read = (readFnPtr)read;
    device->write = (writeFnPtr)write;
    return device->dev;
}

extern "C" void deviceInit()
{
    for (size_t i = 0; i < DEVICE_NR; i++)
    {
        device_t *device = &devices[i];
        strcpy((char *)device->name, "null");
        device->type = DEV_NULL;
        device->subtype = DEV_NULL;
        device->dev = i;
        device->parent = 0;
        device->ioctl = nullptr;
        device->read = nullptr;
        device->write = nullptr;
        listInit(&device->requestList);
        device->direct = DIRECT_UP;
    }
}

device_t *deviceFind(int subtype, idx_t idx)
{
    idx_t nr = 0;
    for (size_t i = 0; i < DEVICE_NR; i++)
    {
        device_t *device = &devices[i];
        if (device->subtype != subtype)
            continue;
        if (nr == idx)
            return device;
        nr++;
    }
    return nullptr;
}

device_t *deviceGet(dev_t dev)
{
    assert(dev < DEVICE_NR, "");
    device_t *device = &devices[dev];
    assert(device->type != DEV_NULL, "");
    return device;
}

// 执行块设备请求
static void doRequest(request_t *req)
{
    switch (req->type)
    {
    case REQ_READ:
        deviceRead(req->dev, req->buf, req->count, req->idx, req->flags);
        break;
    case REQ_WRITE:
        deviceWrite(req->dev, req->buf, req->count, req->idx, req->flags);
        break;
    default:
        panic("req type %d unknown!!!");
        break;
    }
}

// 获得下一个请求
static request_t *requestNextreq(device_t *device, request_t *req)
{
    list_t *list = &device->requestList;

    if (device->direct == DIRECT_UP && req->node.next == &list->tail)
    {
        device->direct = DIRECT_DOWN;
    }
    else if (device->direct == DIRECT_DOWN && req->node.prev == &list->head)
    {
        device->direct = DIRECT_UP;
    }

    void *next = nullptr;
    if (device->direct == DIRECT_UP)
    {
        next = req->node.next;
    }
    else
    {
        next = req->node.prev;
    }

    if (next == &list->head || next == &list->tail)
    {
        return nullptr;
    }
    return ELEMENT_ENTRY(request_t, node, next);
}

// 块设备请求
void deviceRequest(dev_t dev, void *buf, uint8 count, idx_t idx, int flags, uint32 type)
{
    device_t *device = deviceGet(dev);
    assert(device->type = DEV_BLOCK, ""); // 是块设备
    idx_t offset = idx + deviceIoctl(device->dev, DEV_CMD_SECTOR_START, 0, 0);

    if (device->parent)
    {
        device = deviceGet(device->parent);
    }

    request_t *req = (request_t*)kmalloc(sizeof(request_t));

    req->dev = device->dev;
    req->buf = (uint8*)buf;
    req->count = count;
    req->idx = offset;
    req->flags = flags;
    req->type = type;
    req->task = nullptr;
    // 判断列表是否为空
    bool empty = listEmpty(&device->requestList);

    // 将请求压入链表
    // listPush(&device->requestList, &req->node);

    // 将请求插入链表
    listInsertSort(&device->requestList, &req->node, ELEMENT_NODE_OFFSET(request_t, node, idx));

    // 如果列表不为空，则阻塞，因为已经有请求在处理了，等待处理完成；
    if (!empty)
    {
        req->task = runningTask();
        taskBlock(req->task, nullptr, TASK_BLOCKED);
    }

    doRequest(req);
    request_t *nextreq = requestNextreq(device, req);

    listRemove(&req->node);
    kfree(req);

    if (nextreq)
    {
        assert(nextreq->task->magic == SOUL_MAGIC, "");
        taskUnblock(nextreq->task);
    }
}