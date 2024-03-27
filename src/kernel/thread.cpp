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

void testRecursion(int count)
{
    char tmp[0x400];
    printf("stack TEST: %d\n",count);
    testRecursion(count+1);
}

// extern uint32 keyboardRead(char *buf, uint32 count);

static void userInitThread()
{
    uint32 counter = 0;
    int status = 0;
    while (true)
    {
        // testRecursion(1);
        // pid_t pid = fork();

        // if (pid)
        // {
        //     printf("fork after parent %d, %d, %d\n", pid, getpid(), getppid());
        //     pid_t child = waitpid(pid, &status);
        //     printf("wait pid %d status %d %d\n", child, status, time());
        // }
        // else
        // {
        //     printf("fork after child %d, %d, %d\n", pid, getpid(), getppid());
        //     // sleep(1000);
        //     exit(0);
        // }
        sleep(1000);
    }
}

void initThread()
{

    char temp[100]; // 为栈顶有足够的空间
    setInterruptStateTrue();
    test();
    taskToUserMode((target_t)userInitThread);
}

void testThread()
{
    setInterruptStateTrue();
    uint32 counter = 0;

    while (true)
    {
        sleep(8000);
    }
}