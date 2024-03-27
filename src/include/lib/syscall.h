#ifndef SOUL_SYSCALL_H
#define SOUL_SYSCALL_H

#include <types/types.h>

typedef enum syscall_t
{
    SYS_NR_TEST,
    SYS_NR_EXIT = 1,
    SYS_NR_FORK = 2,
    SYS_NR_WRITE = 4,
    SYS_NR_WAITPID = 7,
    SYS_NR_TIME = 13,
    SYS_NR_GETPID = 20,
    SYS_NR_BRK = 45,
    SYS_NR_GETPPID = 64,
    SYS_NR_SLEEP = 158,
    SYS_NR_YIELD = 162,
}syscall_t;

uint32 test();
pid_t fork();
void exit(int32 status);
pid_t waitpid(pid_t pid, int32 *status);
void yield();
void sleep(uint32 ms);
pid_t getpid(); // 获取进程ID
pid_t getppid(); // 获取父进程ID
int32 brk(void *addr);
int32 write(fd_t fd, char *buf, uint32 len);
time_t time();

#endif