#ifndef SOUL_TYPES_H
#define SOUL_TYPES_H

// 用于省略函数的栈帧
#define _ofp __attribute__((optimize("omit-frame-pointer")))

typedef char int8;
typedef short int16;
typedef int int32;
typedef long long int64;

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;

typedef unsigned long long size_t;

#define UINT32_MAX 4294967295
#define UINT64_MAX 18446744073709551615

#define EOS '\0' // 字符串结尾

#endif