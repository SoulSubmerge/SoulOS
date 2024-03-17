#include <kernel/assert.h>
#include <types/args.h>
#include <types/types.h>
#include <lib/stdio.h>
#include <kernel/printk.h>

static char buf[10240];

// 强制阻塞
static void spin(const char *name)
{
    printk("spinning in %s ...\n", name);
    while (true)
        ;
}

void assertionFailure(char const *exp, char const *file, char const *base, int line)
{
    printk(
        "\n--> assert(%s) failed!!!\n"
        "--> file: %s \n"
        "--> base: %s \n"
        "--> line: %d \n",
        exp, file, base, line);

    spin("assertion_failure()");

    // 不可能走到这里，否则出错；
    asm volatile("ud2");
}

void panic(const char *fmt, ...)
{
    var_list args;
    VAR_START_FN(args, fmt);
    int i = vsprintf(buf, fmt, args);
    VAR_END_FN(args);

    printk("!!! panic !!!\n--> %s \n", buf);
    spin("panic()");

    // 不可能走到这里，否则出错；
    asm volatile("ud2");
}