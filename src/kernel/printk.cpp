#include <kernel/printk.h>
#include <kernel/console.h>
#include <lib/stdio.h>
// #include <kernel/debug.h>

static char buf[10240];

int printk(const char *fmt, ...)
{
    var_list args;
    int i = 0;
    VAR_START_FN(args, fmt);
    i = vsprintf(buf, fmt, args);
    VAR_END_FN(args);
    // device_t *device = device_find(DEV_CONSOLE, 0);
    // device_write(device->dev, buf, i, 0, 0);
    consoleWrite(buf,i);

    return i;
}
