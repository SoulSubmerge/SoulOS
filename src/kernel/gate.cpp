#include <kernel/interrupt.h>
#include <kernel/assert.h>
#include <kernel/debug.h>
#include <kernel/logk.h>
#include <lib/syscall.h>
#include <kernel/task.h>
#include <kernel/console.h>
#include <kernel/memory.h>

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
    char *ptr = nullptr;
    BREAK_POINT;
    linkPage(0x2000000);

    BREAK_POINT;

    ptr = (char *) 0x2000000;
    ptr[3] = 'T';
    BREAK_POINT;

    unlinkPage(0x2000000);

    BREAK_POINT;
    return 255;

}

int32 sysWrite(fd_t fd, char *buf, uint32 len)
{
    if (fd == stdout || fd == stderr)
    {
        return consoleWrite(buf, len);
    }
    // todo
    panic("write!!!!");
    return 0;
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
    syscallTable[SYS_NR_WRITE] = (handleFunc)sysWrite;
}