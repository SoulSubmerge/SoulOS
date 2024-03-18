#ifndef SOUL_ASSERT_H
#define SOUL_ASSERT_H

void assertionFailure(const char *exp, const char *file, const char *base, int line, const char *msg);

#define assert(exp, msg) \
    if (exp)        \
        ;           \
    else            \
        assertionFailure(#exp, __FILE__, __BASE_FILE__, __LINE__, msg)

void panic(const char *fmt, ...);

#endif