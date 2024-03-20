#ifndef SOUL_TASK_H
#define SOUL_TASK_H

#include <types/types.h>
#include <lib/bitmap.h>

#define KERNEL_USER 0
#define NORMAL_USER 1

#define TASK_NAME_LEN 16


// 一个任务的栈页大小 4k
#define STACK_FRAME_SIZE 0x1000

typedef uint32* target_t;
typedef void (*eipPtr)(void);
typedef enum task_state_t
{
    TASK_INIT,     // 初始化
    TASK_RUNNING,  // 执行
    TASK_READY,    // 就绪
    TASK_BLOCKED,  // 阻塞
    TASK_SLEEPING, // 睡眠
    TASK_WAITING,  // 等待
    TASK_DIED,     // 死亡
}TASK_STATE_ENUM;

typedef struct task_info
{
    uint32 *stack;
    TASK_STATE_ENUM state;      // 任务状态
    uint32 priority;            // 任务优先级
    uint32 ticks;               // 剩余时间片
    uint32 jiffies;             // 上次执行时全局时间片
    uint32 name[TASK_NAME_LEN]; // 任务名
    uint32 uid;                 // 用户 id
    uint32 pde;                 // 页目录物理地址
    BITMAP_T *vmap;             // 进程虚拟内存位图
    uint32 magic;               // 内核魔数，用于检测栈溢出
}TASK_INFO;

typedef struct task_register_info
{
    uint32 edi;
    uint32 esi;
    uint32 ebx;
    uint32 ebp;
    eipPtr eip;
}TASK_REGISTER_INFO;

void taskInit();
TASK_INFO* runningTask();
void schedule();

#endif