#include <kernel/task.h>
#include <kernel/printk.h>
#include <kernel/debug.h>
#include <lib/stdlib.h>

extern "C" void* getRunningTaskEsp();

// 获取当前执行的任务的栈环境
TASK_INFO* runningTask()
{
    // asm volatile(
    //     "movl %esp, %eax\n"
    //     "andl $0xfffff000, %eax\n");
    return (TASK_INFO*)getRunningTaskEsp();
}

extern "C" void taskSwitch(TASK_INFO *next);

// 调度下一个任务
void scheduler()
{
    TASK_INFO *current = runningTask();
    TASK_INFO *next = current == (TASK_INFO*)(1*STACK_FRAME_SIZE) ? (TASK_INFO*)(2*STACK_FRAME_SIZE) : (TASK_INFO*)(1*STACK_FRAME_SIZE);
    taskSwitch(next);
}

void _ofp threadA()
{
    asm volatile("sti\n");
    while (true)
    {
        printk("Running task A");
        // scheduler();
        delay(1000000);
    }
}

void _ofp threadB()
{
    asm volatile("sti\n");
    while (true)
    {
        printk("Running task B");
        // scheduler();
        delay(1000000);
    }
}

typedef void (*targetFn)(void);
static void taskCreate(TASK_INFO *task, void* target)
{
    uint32 stack = (uint32)task + STACK_FRAME_SIZE;

    stack -= sizeof(TASK_REGISTER_INFO);
    TASK_REGISTER_INFO *frame = (TASK_REGISTER_INFO*)stack;

    frame->ebx = 0x11111111;
    frame->esi = 0x22222222;
    frame->edi = 0x33333333;
    frame->ebp = 0x44444444;
    frame->eip = (targetFn)target;

    task->stack = (uint32*)stack;
}

void taskInit()
{
    taskCreate((TASK_INFO*)(1*STACK_FRAME_SIZE), (void*)threadA);
    taskCreate((TASK_INFO*)(2*STACK_FRAME_SIZE), (void*)threadB);
    scheduler();
}