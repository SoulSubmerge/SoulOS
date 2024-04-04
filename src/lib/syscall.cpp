#include <lib/syscall.h>

static inline uint32 _syscall0(uint32 nr)
{
    uint32 ret;
    asm volatile(
        "int $0x80\n"
        : "=a"(ret)
        : "a"(nr));
    return ret;
}

static inline uint32 _syscall1(uint32 nr, uint32 arg)
{
    uint32 ret;
    asm volatile(
        "int $0x80\n"
        : "=a"(ret)
        : "a"(nr), "b"(arg));
    return ret;
}

static inline uint32 _syscall2(uint32 nr, uint32 arg1, uint32 arg2)
{
    uint32 ret;
    asm volatile(
        "int $0x80\n"
        : "=a"(ret)
        : "a"(nr), "b"(arg1), "c"(arg2));
    return ret;
}

static inline uint32 _syscall3(uint32 nr, uint32 arg1, uint32 arg2, uint32 arg3)
{
    uint32 ret;
    asm volatile(
        "int $0x80\n"
        : "=a"(ret)
        : "a"(nr), "b"(arg1), "c"(arg2), "d"(arg3));
    return ret;
}

uint32 test()
{
    return _syscall0(SYS_NR_TEST);
}

void exit(int32 status)
{
    _syscall1(SYS_NR_EXIT, (uint32)status);
}

pid_t fork()
{
    return _syscall0(SYS_NR_FORK);
}

pid_t waitpid(pid_t pid, int32 *status)
{
    return _syscall2(SYS_NR_WAITPID, pid, (uint32)status);
}

void yield()
{
    _syscall0(SYS_NR_YIELD);
}

void sleep(uint32 ms)
{
    _syscall1(SYS_NR_SLEEP, ms);
}

pid_t getpid()
{
    return _syscall0(SYS_NR_GETPID);
}

pid_t getppid()
{
    return _syscall0(SYS_NR_GETPPID);
}

int32 brk(void *addr)
{
    return _syscall1(SYS_NR_BRK, (uint32)addr);
}

int32 write(fd_t fd, char *buf, uint32 len)
{
    return _syscall3(SYS_NR_WRITE, fd, (uint32)buf, len);
}

time_t time()
{
    return _syscall0(SYS_NR_TIME);
}

mode_t umask(mode_t mask)
{
    return _syscall1(SYS_NR_UMASK, (uint32)mask);
}