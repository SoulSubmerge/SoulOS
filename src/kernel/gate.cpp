#include <kernel/interrupt.h>
#include <kernel/assert.h>
#include <kernel/debug.h>
#include <kernel/logk.h>
#include <lib/syscall.h>
#include <kernel/task.h>
#include <kernel/console.h>
#include <kernel/memory.h>

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


#include <lib/charArray.h>
#include <kernel/ide.h>

extern ide_ctrl_t controllers[2];

static uint32 sysTest()
{
    uint16 *buf = (uint16*)allocKpage(1);
    LOGK("pio read buffer 0x%p\n", buf);
    ide_disk_t *disk = &controllers[0].disks[0];
    idePioRead(disk, buf, 4, 0);
    BREAK_POINT;
    memset(buf, 0x5a, 512);
    idePioWrite(disk, buf, 1, 1);
    freeKpage((uint32)buf, 1);
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

extern time_t sysTime();

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
}