#ifndef SOUL_STDLIB_H
#define SOUL_STDLIB_H

#include <types/types.h>
#define MAX(a, b) (a < b ? b : a)
#define MIN(a, b) (a < b ? a : b)
#define ABS(a) (a < 0 ? -a : a)

void delay(uint64 _count);
void hang();

char toupper(char _ch);
char tolower(char _ch);

uint8 bcdToBin(uint8 _value);
uint8 binToBcd(uint8 _value);

uint32 divRoundUp(uint32 _num, uint32 _size);

bool isdigit(int32 _c);

int32 atoi(const char *_str);

#endif