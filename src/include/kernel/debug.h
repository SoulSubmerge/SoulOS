
#ifndef SOUL_DEBUG_H
#define SOUL_DEBUG_H

void debugk(const char *file, int line, const char *fmt, ...);

#define BREAK_POINT asm volatile("xchgw %bx, %bx") // bochs magic breakpoint
#define DEBUGK(fmt, args...) debugk(__BASE_FILE__, __LINE__, fmt, ##args)

// #define LOGK(fmt, args...) DEBUGK(fmt, ##args)

#endif
