#include <kernel/task.h>
#include <kernel/printk.h>
#include <kernel/debug.h>
#include <lib/stdlib.h>
#include <kernel/memory.h>
#include <kernel/assert.h>
#include <kernel/interrupt.h>
#include <kernel/kernel.h>
#include <lib/charArray.h>
#include <lib/syscall.h>

extern BITMAP_T kernelMap;

extern "C" void* getRunningTaskEsp();
extern "C" void taskSwitch(TASK_INFO *next);

#define NR_TASKS 64
extern uint32 volatile jiffies;
extern uint32 jiffy;
static TASK_INFO *taskTable[NR_TASKS]; // 任务表
static LIST_T blockList; // 任务阻塞的链表
static LIST_T  sleepList; // 任务睡眠链表
static TASK_INFO *idleTask; // 空闲进程的任务，解决没有其它工作进程执行的情况

// 从 task_table 里获得一个空闲的任务
static TASK_INFO *getFreeTask()
{
    for (size_t i = 0; i < NR_TASKS; i++)
    {
        if (taskTable[i] == nullptr)
        {
            taskTable[i] = (TASK_INFO*)allocKpage(1); // todo free_kpage
            return taskTable[i];
        }
    }
    panic("No more tasks");
    return nullptr;
}


// 从任务数组中查找某种状态的任务，自己除外
static TASK_INFO *taskSearch(TASK_STATE_ENUM state)
{
    assert(!getInterruptState(), "Undisabled interrupt.");
    TASK_INFO *task = nullptr;
    TASK_INFO *current = runningTask();

    for (size_t i = 0; i < NR_TASKS; i++)
    {
        TASK_INFO *ptr = taskTable[i];
        if (ptr == nullptr)
            continue;

        if (ptr->state != state)
            continue;
        if (current == ptr)
            continue;
        if (task == nullptr || task->ticks < ptr->ticks || ptr->jiffies < task->jiffies)
            task = ptr;
    }

    if(task == nullptr && state == TASK_READY)
    {
        task = idleTask; // 找到不其它就绪任务就把空闲进程扔给CPU处理
    }

    return task;
}

void taskYield()
{
    schedule();
}

// 获取当前执行的任务的栈环境
TASK_INFO* runningTask()
{
    TASK_INFO* taskEspPtr = (TASK_INFO*)getRunningTaskEsp();
    return taskEspPtr;
}

// 调度下一个任务
void schedule()
{
    assert(!getInterruptState(), "Undisabled interrupt."); // 不可中断
    TASK_INFO *current = runningTask();
    TASK_INFO *next = taskSearch(TASK_READY);

    assert(next != nullptr, "Invalid task address.");
    assert(next->magic == SOUL_MAGIC, "Task page stack overflow causes task PCB information to be damaged.");

    if (current->state == TASK_RUNNING)
    {
        current->state = TASK_READY;
    }

    if(!current->ticks)
    {
        current->ticks = current->priority;
    }

    next->state = TASK_RUNNING;
    if (next == current)
        return;

    taskSwitch(next);
}

static TASK_INFO *taskCreate(target_t target, const char *name, uint32 priority, uint32 uid)
{
    // BREAK_POINT;
    TASK_INFO *task = getFreeTask();
    // BREAK_POINT;
    memset(task, 0, PAGE_SIZE);
    uint32 stack = (uint32)task + PAGE_SIZE;
    stack -= sizeof(TASK_REGISTER_INFO);
    TASK_REGISTER_INFO *frame = (TASK_REGISTER_INFO*)stack;
    frame->ebx = 0x11111111;
    frame->esi = 0x22222222;
    frame->edi = 0x33333333;
    frame->ebp = 0x44444444;
    frame->eip = (eipPtr)target;
    strcpy((char *)task->name, name);
    task->stack = (uint32*)stack;
    task->priority = priority;
    task->ticks = task->priority;
    task->jiffies = 0;
    task->state = TASK_READY;
    task->uid = uid;
    task->vmap = &kernelMap;
    task->pde = KERNEL_PAGE_DIR;
    task->magic = SOUL_MAGIC;
    printk("magic: %p address: %p\n", task->magic, task);
    // BREAK_POINT;
    return task;
}

static void taskSetup()
{
    TASK_INFO *task = runningTask();
    task->magic = SOUL_MAGIC;
    task->ticks = 1;
    memset(taskTable, 0, sizeof(taskTable));
}

// 任务阻塞
void taskBlock(TASK_INFO *task, LIST_T *blist, TASK_STATE_ENUM state)
{
    assert(!getInterruptState(), "Undisabled interrupt.");
    assert(task->node.next == nullptr, "The rear drive of the node to be inserted is not empty.");
    assert(task->node.prev == nullptr, "The precursor of the node to be inserted is not empty.");

    if (blist == nullptr)
    {
        blist = &blockList;
    }

    listPush(blist, &task->node);

    assert(state != TASK_READY && state != TASK_RUNNING, "The task status cannot be Executing or Ready.");

    task->state = state;

    TASK_INFO *current = runningTask();
    if (current == task)
    {
        schedule();
    }
}

// 解除任务阻塞
void taskUnblock(TASK_INFO *task)
{
    assert(!getInterruptState(), "Undisabled interrupt.");

    listRemove(&task->node);

    assert(task->node.next == nullptr, "The rear drive of the node to be inserted is not empty.");
    assert(task->node.prev == nullptr, "The precursor of the node to be inserted is not empty.");

    task->state = TASK_READY;
}

void taskSleep(uint32 ms)
{
    assert(!getInterruptState(), "Undisabled interrupt."); // 不可中断

    uint32 ticks = ms / jiffy;        // 需要睡眠的时间片
    ticks = ticks > 0 ? ticks : 1; // 至少休眠一个时间片

    // 记录目标全局时间片，在那个时刻需要唤醒任务
    TASK_INFO *current = runningTask();
    current->ticks = jiffies + ticks;
    // printk("Sleep ticks: %d\n", jiffies);

    // 从睡眠链表找到第一个比当前任务唤醒时间点更晚的任务，进行插入排序
    LIST_T *list = &sleepList;
    LIST_NODE_T *anchor = &list->tail;

    for (LIST_NODE_T *ptr = list->head.next; ptr != &list->tail; ptr = ptr->next)
    {
        TASK_INFO *task = ELEMENT_ENTRY(TASK_INFO, node, ptr);

        if (task->ticks > current->ticks)
        {
            anchor = ptr;
            break;
        }
    }

    assert(current->node.next == nullptr, "The rear drive of the node to be inserted is not empty.");
    assert(current->node.prev == nullptr, "The precursor of the node to be inserted is not empty.");

    // 插入链表
    listInsertBefore(anchor, &current->node);

    // 阻塞状态是睡眠
    current->state = TASK_SLEEPING;

    // 调度执行其他任务
    schedule();
}

void taskWakeup()
{
    assert(!getInterruptState(), "Undisabled interrupt."); // 不可中断

    // 从睡眠链表中找到 ticks 小于等于 jiffies 的任务，恢复执行
    LIST_T *list = &sleepList;
    for (LIST_NODE_T *ptr = list->head.next; ptr != &list->tail;)
    {
        TASK_INFO *task = ELEMENT_ENTRY(TASK_INFO, node, ptr);
        if (task->ticks > jiffies)
        {
            break;
        }

        // unblock 会将指针清空
        ptr = ptr->next;

        task->ticks = 0;
        taskUnblock(task);
    }
}

extern void idleThread(); // 空闲进程的线程任务
extern void initThread(); // 初始化进程的线程任务

extern void testThread();

void taskInit()
{
    listInit(&blockList);
    listInit(&sleepList);
    taskSetup();
    idleTask = taskCreate((target_t)idleThread,"idle",1,KERNEL_USER);
    taskCreate((target_t)initThread, "init", 5, NORMAL_USER);
    taskCreate((target_t)testThread, "test", 5, KERNEL_USER);
}

