#ifndef SOUL_CHAR_ARRAY_H
#define SOUL_CHAR_ARRAY_H

#include <types/types.h>


char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, size_t count);
char *strcat(char *dest, const char *src);
size_t strlen(const char *str);
size_t strnlen(const char *str, size_t maxlen);
int32 strcmp(const char *lhs, const char *rhs);
char *strchr(const char *str, int32 ch);
char *strrchr(const char *str, int32 ch);
char *strsep(const char *str);
char *strrsep(const char *str);

int32 memcmp(const void *lhs, const void *rhs, size_t count);
void *memset(void *dest, int32 ch, size_t count);
void *memcpy(void *dest, const void *src, size_t count);
void *memchr(const void *ptr, int32 ch, size_t count);

#endif