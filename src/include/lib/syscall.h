#ifndef SOUL_SYSCALL_H
#define SOUL_SYSCALL_H

#include <types/types.h>

typedef enum syscall_t
{
    SYS_NR_TEST,
    SYS_NR_SLEEP,
    SYS_NR_YIELD,
}syscall_t;

uint32 test();
void yield();
void sleep(uint32 ms);

#endif