#include <kernel/interrupt.h>
#include <kernel/assert.h>
#include <kernel/debug.h>
#include <kernel/logk.h>
#include <lib/syscall.h>
#include <kernel/task.h>

#define SYSCALL_SIZE 64

handleFunc syscallTable[SYSCALL_SIZE];
TASK_INFO *task = nullptr;


extern "C" void syscallCheck(uint32 nr)
{
    if (nr >= SYSCALL_SIZE)
    {
        panic("syscall nr error!!!");
    }
}

static void sysDefault()
{
    panic("syscall not implemented!!!");
}

static uint32 sysTest()
{
    // LOGK("syscall test...\n");
    if(!task)
    {
        task = runningTask();
        taskBlock(task, nullptr, TASK_BLOCKED);
    }
    else
    {
        taskUnblock(task);
        task = nullptr;
    }
    return 255;
}


void syscallInit()
{
    for (size_t i = 0; i < SYSCALL_SIZE; i++)
    {
        syscallTable[i] = (handleFunc)sysDefault;
    }

    syscallTable[SYS_NR_TEST] = (handleFunc)sysTest;
    syscallTable[SYS_NR_SLEEP] = (handleFunc)taskSleep;
    syscallTable[SYS_NR_YIELD] = (handleFunc)taskYield;
}