#ifndef SOUL_TASK_H
#define SOUL_TASK_H

#include <types/types.h>

// 一个任务的栈页大小 4k
#define STACK_FRAME_SIZE 0x1000

typedef struct task_info
{
    uint32 *stack;
}TASK_INFO;

typedef struct task_register_info
{
    uint32 edi;
    uint32 esi;
    uint32 ebx;
    uint32 ebp;
    void (*eip)(void);
}TASK_REGISTER_INFO;

void taskInit();


#endif