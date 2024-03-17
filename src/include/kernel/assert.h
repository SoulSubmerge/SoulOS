#ifndef SOUL_ASSERT_H
#define SOUL_ASSERT_H

void assertionFailure(const char *exp, const char *file, const char *base, int line);

#define assert(exp) \
    if (exp)        \
        ;           \
    else            \
        assertionFailure(#exp, __FILE__, __BASE_FILE__, __LINE__)

void panic(const char *fmt, ...);

#endif