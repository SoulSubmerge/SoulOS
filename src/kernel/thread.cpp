#include <kernel/interrupt.h>
#include <lib/syscall.h>
#include <kernel/logk.h>
#include <kernel/mutex.h>

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


void initThread()
{

    setInterruptStateTrue();
    while (true)
    {
        // LOGK("init task....\n");
        sleep(500);
    }
}

void testThread()
{
    setInterruptStateTrue();
    uint32 counter = 0;

    while (true)
    {
        // LOGK("test task %d....\n", counter++);
        sleep(669);
    }
}