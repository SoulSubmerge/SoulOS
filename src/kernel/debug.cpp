#include <kernel/debug.h>
#include <types/args.h>
#include <lib/stdio.h>
#include <kernel/printk.h>
// #include <kernel/device.h>

static char buf[10240];

void debugk(const char *file, int line, const char *fmt, ...)
{
    
    // int i = sprintf(buf, "[%s] [%d] ", file, line);
    // printk("[%s] [%d] %s\n", file, line, buf);

    var_list args;
    VAR_START_FN(args, fmt);
    int i = vsprintf(buf, fmt, args);
    VAR_END_FN(args);
    printk("[%s] [%d] %s\n", file, line, buf);
}