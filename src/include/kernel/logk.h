#ifndef SOUL_LOGK_H
#define SOUL_LOGK_H
#include <kernel/printk.h>

#define LOGK(fmt, args...) printk(fmt, ##args)

#endif