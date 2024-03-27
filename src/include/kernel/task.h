#ifndef SOUL_TASK_H
#define SOUL_TASK_H

#include <types/types.h>
#include <lib/bitmap.h>
#include <lib/list.h>

#define KERNEL_USER 0
#define NORMAL_USER 1

#define TASK_NAME_LEN 16

typedef void (*target_t)(void);
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
}task_state_t;

typedef struct task_t
{
    uint32 *stack;              // 内核栈
    list_node_t node;           // 任务阻塞节点
    task_state_t state;      // 任务状态
    uint32 priority;            // 任务优先级
    uint32 ticks;               // 剩余时间片
    uint32 jiffies;             // 上次执行时全局时间片
    char name[TASK_NAME_LEN];   // 任务名
    uint32 uid;                 // 用户 id
    pid_t pid;                  // 任务ID
    pid_t ppid;                 // 父任务ID
    uint32 pde;                 // 页目录物理地址
    bitmap_t *vmap;             // 进程虚拟内存位图
    uint32 brk;
    int32 status;               // 进程特殊状态
    pid_t waitpid;            // 进程等待的 pid
    uint32 magic;               // 内核魔数，用于检测栈溢出
}task_t;

typedef struct task_frame_t
{
    uint32 edi;
    uint32 esi;
    uint32 ebx;
    uint32 ebp;
    eipPtr eip;
}task_frame_t;


typedef struct intr_frame_t
{
    uint32 vector;

    uint32 edi;
    uint32 esi;
    uint32 ebp;
    // 虽然 pushad 把 esp 也压入，但 esp 是不断变化的，所以会被 popad 忽略
    uint32 esp_dummy;

    uint32 ebx;
    uint32 edx;
    uint32 ecx;
    uint32 eax;

    uint32 gs;
    uint32 fs;
    uint32 es;
    uint32 ds;

    uint32 vector0;

    uint32 error;

    uint32 eip;
    uint32 cs;
    uint32 eflags;
    uint32 esp;
    uint32 ss;
}intr_frame_t;


void taskInit();


task_t* runningTask();
void schedule();
void taskExit(int32 status);
pid_t taskFork();
pid_t taskWaitpid(pid_t pid, int32 *status);
void taskYield();
void taskBlock(task_t *task, list_t *blist, task_state_t state); // 任务阻塞
void taskUnblock(task_t *task); // 解除阻塞，就绪
void taskSleep(uint32 ms); // 睡眠
void taskWakeup(); // 苏醒

void taskToUserMode(target_t target);

pid_t sysGetpid();
pid_t sysGetppid();

#endif