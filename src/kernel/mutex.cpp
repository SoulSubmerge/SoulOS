#include <kernel/mutex.h>
#include <kernel/assert.h>
#include <kernel/interrupt.h>
#include <kernel/kernel.h>

void mutexInit(mutex_t *mutex)
{
    mutex->value = false; // 初始化时没有被人持有
    listInit(&mutex->waiters);
}

// 尝试持有互斥量
void mutexLock(mutex_t *mutex)
{
    // 关闭中断，保证原子操作
    bool intr = interruptDisable();

    task_t *current = runningTask();
    while (mutex->value == true)
    {
        // 若 value 为 true，表示已经被别人持有
        // 则将当前任务加入互斥量等待队列
        taskBlock(current, &mutex->waiters, TASK_BLOCKED);
    }

    // 无人持有
    assert(mutex->value == false, "The expected mutex is not held by anyone.");

    // 持有
    mutex->value = true;
    assert(mutex->value == true, "The mutex is expected to be held by someone.");

    // 恢复之前的中断状态
    setInterruptState(intr);
}

// 释放互斥量
void mutexUnlock(mutex_t *mutex)
{
    // 关闭中断，保证原子操作
    bool intr = interruptDisable();

    // 已持有互斥量
    assert(mutex->value == true, "The mutex is expected to be held.");

    // 取消持有
    mutex->value = false;
    assert(mutex->value == false,  "The expected mutex is not held by anyone.");

    // 如果等待队列不为空，则恢复执行
    if (!listEmpty(&mutex->waiters))
    {
        task_t *task = ELEMENT_ENTRY(task_t, node, mutex->waiters.tail.prev);
        assert(task->magic == SOUL_MAGIC, "Task page stack overflow causes task PCB information to be damaged.");
        taskUnblock(task);
        // 保证新进程能获得互斥量，不然可能饿死
        taskYield();
    }

    // 恢复之前的中断状态
    setInterruptState(intr);
}

// 锁初始化
void lockInit(lock_t *lock)
{
    lock->holder = nullptr;
    lock->repeat = 0;
    mutexInit(&lock->mutex);
}

// 尝试持有锁
void lockAcquire(lock_t *lock)
{
    task_t *current = runningTask();
    if (lock->holder != current)
    {
        mutexLock(&lock->mutex);
        lock->holder = current;
        assert(lock->repeat == 0, "The expected lock is not held.");
        lock->repeat = 1;
    }
    else
    {
        lock->repeat++;
    }
}

// 释放锁
void lockRelease(lock_t *lock)
{
    task_t *current = runningTask();
    assert(lock->holder == current, "The current thread is expected to be the holder of the lock.");
    if (lock->repeat > 1)
    {
        lock->repeat--;
        return;
    }

    assert(lock->repeat == 1, "The expectation lock is held by one person.");

    lock->holder = nullptr;
    lock->repeat = 0;
    mutexUnlock(&lock->mutex);
}