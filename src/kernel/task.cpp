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
#include <kernel/gdt.h>
#include <kernel/arena.h>
#include <kernel/logk.h>

extern bitmap_t kernelMap;

extern "C" void* getRunningTaskEsp();
extern "C" void taskSwitch(task_t *next);

#define NR_TASKS 64
extern uint32 volatile jiffies;
extern uint32 jiffy;
static task_t *taskTable[NR_TASKS]; // 任务表
static list_t blockList; // 任务阻塞的链表
static list_t  sleepList; // 任务睡眠链表
static task_t *idleTask; // 空闲进程的任务，解决没有其它工作进程执行的情况

extern TSS_T m_tss;

// 从 task_table 里获得一个空闲的任务
static task_t *getFreeTask()
{
    for (size_t i = 0; i < NR_TASKS; i++)
    {
        if (taskTable[i] == nullptr)
        {
            task_t *task = (task_t *)allocKpage(1); // todo free_kpage
            memset(task, 0, PAGE_SIZE);
            task->pid = i;
            taskTable[i] = task;
            return task;
        }
    }
    panic("No more tasks");
    return nullptr;
}

// 获取进程 id
pid_t sysGetpid()
{
    task_t *task = runningTask();
    return task->pid;
}

// 获取父进程 id
pid_t sysGetppid()
{
    task_t *task = runningTask();
    return task->ppid;
}



// 从任务数组中查找某种状态的任务，自己除外
static task_t *taskSearch(task_state_t state)
{
    assert(!getInterruptState(), "Undisabled interrupt.");
    task_t *task = nullptr;
    task_t *current = runningTask();

    for (size_t i = 0; i < NR_TASKS; i++)
    {
        task_t *ptr = taskTable[i];
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

// 任务阻塞
void taskBlock(task_t *task, list_t *blist, task_state_t state)
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

    task_t *current = runningTask();
    if (current == task)
    {
        schedule();
    }
}

// 解除任务阻塞
void taskUnblock(task_t *task)
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
    task_t *current = runningTask();
    current->ticks = jiffies + ticks;
    // printk("Sleep ticks: %d\n", jiffies);

    // 从睡眠链表找到第一个比当前任务唤醒时间点更晚的任务，进行插入排序
    list_t *list = &sleepList;
    list_node_t *anchor = &list->tail;

    for (list_node_t *ptr = list->head.next; ptr != &list->tail; ptr = ptr->next)
    {
        task_t *task = ELEMENT_ENTRY(task_t, node, ptr);

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
    list_t *list = &sleepList;
    for (list_node_t *ptr = list->head.next; ptr != &list->tail;)
    {
        task_t *task = ELEMENT_ENTRY(task_t, node, ptr);
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

// 激活任务
void taskActivate(task_t *task)
{
    assert(task->magic == SOUL_MAGIC, "Task page stack overflow causes task PCB information to be damaged.");
    if(task->pde != getCr3())
    {
        setCr3(task->pde);
    }

    if (task->uid != KERNEL_USER)
    {
        m_tss.esp0 = (uint32)task + PAGE_SIZE;
    }
}

// 获取当前执行的任务的栈环境
task_t* runningTask()
{
    task_t* taskEspPtr = (task_t*)getRunningTaskEsp();
    return taskEspPtr;
}

// 调度下一个任务
void schedule()
{
    assert(!getInterruptState(), "Undisabled interrupt."); // 不可中断
    task_t *current = runningTask();
    task_t *next = taskSearch(TASK_READY);

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

    taskActivate(next); // taskActivate 一定要在 taskSwitch 之前调用
    taskSwitch(next);
}

static task_t *taskCreate(target_t target, const char *name, uint32 priority, uint32 uid)
{
    task_t *task = getFreeTask();
    uint32 stack = (uint32)task + PAGE_SIZE;
    stack -= sizeof(task_frame_t);
    task_frame_t *frame = (task_frame_t*)stack;
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
    task->brk = KERNEL_MEMORY_SIZE;
    task->magic = SOUL_MAGIC;
    return task;
}

// 调用该函数的地方不能有任何局部变量
// 调用前栈顶需要准备足够的空间
void taskToUserMode(target_t target)
{
    task_t *task = runningTask();

    // 创建用户进程虚拟内存位图
    task->vmap = (bitmap_t*)kmalloc(sizeof(bitmap_t)); // todo kfree
    void *buf = (void *)allocKpage(1);     // todo free_kpage
    bitmapInit(task->vmap, (char*)buf, PAGE_SIZE, KERNEL_MEMORY_SIZE / PAGE_SIZE);

    // 创建用户进程页表
    task->pde = (uint32)copyPde();
    setCr3(task->pde);

    uint32 addr = (uint32)task + PAGE_SIZE;

    addr -= sizeof(intr_frame_t);
    intr_frame_t *iframe = (intr_frame_t *)(addr);
    iframe->vector = 0x20;
    iframe->edi = 1;
    iframe->esi = 2;
    iframe->ebp = 3;
    iframe->esp_dummy = 4;
    iframe->ebx = 5;
    iframe->edx = 6;
    iframe->ecx = 7;
    iframe->eax = 8;

    iframe->gs = 0;
    iframe->ds = USER_DATA_SELECTOR;
    iframe->es = USER_DATA_SELECTOR;
    iframe->fs = USER_DATA_SELECTOR;
    iframe->ss = USER_DATA_SELECTOR;
    iframe->cs = USER_CODE_SELECTOR;

    iframe->error = SOUL_MAGIC;

    iframe->eip = (uint32)target;
    iframe->eflags = (0 << 12 | 0b10 | 1 << 9);
    iframe->esp = USER_STACK_TOP;
    // printk("esp-esp: %p %p %p\n", iframe->esp, iframe, target);
    asm volatile(
        "movl %0, %%esp\n"
        "jmp interruptExitFn\n" ::"m"(iframe));
}

extern "C" void interruptExitFn();

static void taskBuildStack(task_t *task)
{
    uint32 addr = (uint32)task + PAGE_SIZE;
    addr -= sizeof(intr_frame_t);
    intr_frame_t *iframe = (intr_frame_t *)addr;
    iframe->eax = 0;

    addr -= sizeof(task_frame_t);
    task_frame_t *frame = (task_frame_t*)addr;

    frame->ebp = 0xaa55aa55;
    frame->ebx = 0xaa55aa55;
    frame->edi = 0xaa55aa55;
    frame->esi = 0xaa55aa55;

    frame->eip = interruptExitFn;

    task->stack = (uint32 *)frame;
}

pid_t taskFork()
{
    task_t *task = runningTask();

    // 当前进程没有阻塞，且正在执行
    assert(task->node.next == nullptr && task->node.prev == nullptr && task->state == TASK_RUNNING, "");

    // 拷贝内核栈 和 PCB
    task_t *child = getFreeTask();
    pid_t pid = child->pid;
    memcpy(child, task, PAGE_SIZE);

    child->pid = pid;
    child->ppid = task->pid;
    child->ticks = child->priority;
    child->state = TASK_READY;

    // 拷贝用户进程虚拟内存位图
    child->vmap = (bitmap_t*)kmalloc(sizeof(bitmap_t)); // todo kfree
    memcpy(child->vmap, task->vmap, sizeof(bitmap_t));

    // 拷贝虚拟位图缓存
    void *buf = (void *)allocKpage(1); // todo free_kpage
    memcpy(buf, task->vmap->bits, PAGE_SIZE);
    child->vmap->bits = (uint8*)buf;

    // 拷贝页目录
    child->pde = (uint32)copyPde();

    // 构造 child 内核栈
    taskBuildStack(child); // ROP
    // schedule();

    return child->pid;
}

void taskExit(int status)
{
    task_t *task = runningTask();

    // 当前进程没有阻塞，且正在执行
    assert(task->node.next == nullptr && task->node.prev == nullptr && task->state == TASK_RUNNING, "");

    task->state = TASK_DIED;
    task->status = status;

    freePde();

    freeKpage((uint32)task->vmap->bits, 1);
    kfree(task->vmap);

    // 将子进程的父进程赋值为自己的父进程
    for (size_t i = 2; i < NR_TASKS; i++)
    {
        task_t *child = taskTable[i];
        if (!child)
            continue;
        if (child->ppid != task->pid)
            continue;
        child->ppid = task->ppid;
    }

    task_t *parent = taskTable[task->ppid];
    if (parent->state == TASK_WAITING && (parent->waitpid == -1 || parent->waitpid == task->pid))
    {
        taskUnblock(parent);
    }

    schedule();
}


pid_t taskWaitpid(pid_t pid, int32 *status)
{
    task_t *task = runningTask();
    task_t *child = nullptr;

    while (true)
    {
        bool has_child = false;
        for (size_t i = 2; i < NR_TASKS; i++)
        {
            task_t *ptr = taskTable[i];
            if (!ptr)
                continue;

            if (ptr->ppid != task->pid)
                continue;
            if (pid != ptr->pid && pid != -1)
                continue;

            if (ptr->state == TASK_DIED)
            {
                child = ptr;
                taskTable[i] = nullptr;
                // goto rollback;

                *status = child->status;
                uint32 ret = child->pid;
                freeKpage((uint32)child, 1);
                return ret;
            }

            has_child = true;
        }
        if (has_child)
        {
            task->waitpid = pid;
            taskBlock(task, nullptr, TASK_WAITING);
            continue;
        }
        break;
    }

    // 没找到符合条件的子进程
    return -1;

// rollback:
//     *status = child->status;
//     uint32 ret = child->pid;
//     freeKpage((uint32)child, 1);
//     return ret;
}

static void taskSetup()
{
    task_t *task = runningTask();
    task->magic = SOUL_MAGIC;
    task->ticks = 1;
    memset(taskTable, 0, sizeof(taskTable));
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
    taskCreate((target_t)testThread, "test", 5, KERNEL_USER);
    taskCreate((target_t)testThread, "test", 5, KERNEL_USER);
}

