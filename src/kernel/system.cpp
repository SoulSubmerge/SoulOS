#include <kernel/task.h>
#include <kernel/logk.h>

mode_t sysUmask(mode_t mask)
{
    task_t *task = runningTask();
    mode_t old = task->umask;
    task->umask = mask & 0777;
    LOGK("new umask id: %d %d\n", task->umask, 0777);
    return old;
}