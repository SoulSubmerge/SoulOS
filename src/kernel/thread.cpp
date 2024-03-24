#include <kernel/interrupt.h>
#include <lib/syscall.h>
#include <kernel/logk.h>
#include <kernel/mutex.h>
#include <kernel/task.h>
#include <kernel/debug.h>
#include <lib/stdio.h>
#include <kernel/arena.h>

void idleThread()
{
    setInterruptStateTrue();
    uint32 counter = 0;
    while(true)
    {
        // LOGK("idle task.... %d\n", counter++);
        asm volatile(
            "sti\n" // 开中断
            "hlt\n" // 关闭 CPU，进入暂停状态，等待外中断的到来
        );
        yield(); // 放弃执行权，调度执行其他任务
    }
}

// extern uint32 keyboardRead(char *buf, uint32 count);

static void userInitThread()
{
    uint32 counter = 0;
    char ch;
    while (true)
    {
        // BREAK_POINT;
        // uint32 counter = 0;
        // asm volatile("in $0x92, %ax\n");
        test();
        sleep(1000);
        // printf("task is in user mode %d\n", counter++);
    }
}

void initThread()
{

    char temp[100]; // 为栈顶有足够的空间
    taskToUserMode((target_t)userInitThread);
}

void testThread()
{
    setInterruptStateTrue();
    uint32 counter = 0;

    while (true)
    {
        sleep(2000);
    }
}