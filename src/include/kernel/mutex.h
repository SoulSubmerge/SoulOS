#ifndef SOUL_MUTEX_H
#define SOUL_MUTEX_H

#include <types/types.h>
#include <kernel/task.h>
#include <lib/list.h>


typedef struct mutex_t
{
    bool value; // 互斥信号量
    list_t waiters; // 等待队列
}mutex_t;

typedef struct lock_t
{
    task_t *holder;     // 持有者
    mutex_t mutex;     // 互斥量
    uint32 repeat;     // 重入次数
}lock_t;

void mutexInit(mutex_t *mutex);   // 初始化互斥量
void mutexLock(mutex_t *mutex);   // 尝试持有互斥量
void mutexUnlock(mutex_t *mutex); // 释放互斥量

void lockInit(lock_t *lock);   // 锁初始化
void lockAcquire(lock_t *lock);   // 加锁
void lockRelease(lock_t *lock); // 解锁

#endif