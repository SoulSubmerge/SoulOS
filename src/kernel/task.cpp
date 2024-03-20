#include <kernel/task.h>
#include <kernel/printk.h>
#include <kernel/debug.h>
#include <lib/stdlib.h>
#include <kernel/memory.h>
#include <kernel/assert.h>
#include <kernel/interrupt.h>
#include <kernel/kernel.h>
#include <lib/charArray.h>

extern BITMAP_T kernelMap;

extern "C" void* getRunningTaskEsp();
extern "C" void taskSwitch(TASK_INFO *next);

#define NR_TASKS 64
static TASK_INFO *taskTable[NR_TASKS];

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

    return task;
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
    TASK_INFO *current = runningTask();
    TASK_INFO *next = taskSearch(TASK_READY);

    assert(next != nullptr, "Invalid task address.");
    assert(next->magic == SOUL_MAGIC, "Task page stack overflow causes task PCB information to be damaged.");

    if (current->state == TASK_RUNNING)
    {
        current->state = TASK_READY;
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

void thread_a()
{
    setInterruptStateTrue();
    while (true)
    {
        setInterruptStateFalse();
        printk("A");
        setInterruptStateTrue();
    }
}

void thread_b()
{
    setInterruptStateTrue();
    while (true)
    {
        setInterruptStateFalse();
        printk("B");
        setInterruptStateTrue();
    }
}

void thread_c()
{
    setInterruptState(true);
    while (true)
    {
        setInterruptStateFalse();
        printk("C");
        setInterruptStateTrue();
    }
}

void taskInit()
{
    taskSetup();
    taskCreate((target_t)thread_a, "a", 5, KERNEL_USER);
    taskCreate((target_t)thread_b, "b", 5, KERNEL_USER);
    taskCreate((target_t)thread_c, "c", 5, KERNEL_USER);
}