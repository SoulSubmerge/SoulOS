#ifndef SOUL_STDIO_H
#define SOUL_STDIO_H
#include <types/args.h>

int vsprintf(char *buf, const char *fmt, var_list args);
int sprintf(char *buf, const char *fmt, ...);
// int printf(const char *fmt, ...);

#endif