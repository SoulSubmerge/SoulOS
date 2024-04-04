#include <kernel/interrupt.h>
#include <kernel/assert.h>
#include <kernel/debug.h>
#include <kernel/logk.h>
#include <lib/syscall.h>
#include <kernel/task.h>
#include <kernel/console.h>
#include <kernel/memory.h>
#include <kernel/device.h>
#include <lib/charArray.h>
#include <kernel/buffer.h>

#define SYSCALL_SIZE 256

handleFunc syscallTable[SYSCALL_SIZE];
task_t *task = nullptr;


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

int bbb = 2048;

static uint32 sysTest()
{
    char ch;
    device_t *device;
    device = deviceFind(DEV_KEYBOARD, 0);
    assert(device, "");
    deviceRead(device->dev, &ch, 1, 0, 0);
    device = deviceFind(DEV_CONSOLE, 0);
    assert(device, "");
    deviceWrite(device->dev, &ch, 1, 0, 0);
    return 255;
}

int32 sysWrite(fd_t fd, char *buf, uint32 len)
{
    if (fd == stdout || fd == stderr)
    {
        return consoleWrite(nullptr, buf, len);
    }
    // todo
    panic("write!!!!");
    return 0;
}

extern time_t sysTime();
extern mode_t sysUmask(mode_t mask);

void syscallInit()
{
    for (size_t i = 0; i < SYSCALL_SIZE; i++)
    {
        syscallTable[i] = (handleFunc)sysDefault;
    }

    syscallTable[SYS_NR_TEST] = (handleFunc)sysTest;
    syscallTable[SYS_NR_EXIT] = (handleFunc)taskExit;
    syscallTable[SYS_NR_FORK] = (handleFunc)taskFork;
    syscallTable[SYS_NR_WAITPID] = (handleFunc)taskWaitpid;
    syscallTable[SYS_NR_SLEEP] = (handleFunc)taskSleep;
    syscallTable[SYS_NR_YIELD] = (handleFunc)taskYield;
    syscallTable[SYS_NR_GETPID] = (handleFunc)sysGetpid;
    syscallTable[SYS_NR_GETPPID] = (handleFunc)sysGetppid;
    syscallTable[SYS_NR_BRK] = (handleFunc)sysBrk;
    syscallTable[SYS_NR_WRITE] = (handleFunc)sysWrite;
    syscallTable[SYS_NR_TIME] = (handleFunc)sysTime;
    syscallTable[SYS_NR_UMASK] = (handleFunc)sysUmask;
}