#ifndef SOUL_TYPES_H
#define SOUL_TYPES_H

#include <lib/errno.h>

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

typedef unsigned int time_t;
typedef unsigned int idx_t;

typedef int32 pid_t;
typedef int32 dev_t;

#define UINT32_MAX 4294967295
#define UINT64_MAX 18446744073709551615

#define EOF -1 // END OF FILE

#define EOS '\0' // 字符串结尾

#define CONCAT(x, y) x##y
#define RESERVED_TOKEN(x, y) CONCAT(x, y)
#define RESERVED RESERVED_TOKEN(reserved, __LINE__)

typedef int32 fd_t;
typedef uint16 mode_t; // 文件权限

typedef enum std_fd_t
{
    stdin,
    stdout,
    stderr,
} std_fd_t;

#endif