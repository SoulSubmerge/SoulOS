#ifndef SOUL_BITMAP_H
#define SOUL_BITMAP_H

#include <types/types.h>

typedef struct bitmap_t
{
    uint8 *bits;   // 位图缓冲区
    uint32 length; // 位图缓冲区长度
    uint32 offset; // 位图开始的偏移
}bitmap_t;

// 初始化位图
void bitmapInit(bitmap_t *map, char *bits, uint32 length, uint32 offset);

// 构造位图
void bitmapMake(bitmap_t *map, char *bits, uint32 length, uint32 offset);

// 测试位图的某一位是否为 1
bool bitmapTest(bitmap_t *map, uint32 index);

// 设置位图某位的值
void bitmapSet(bitmap_t *map, uint32 index, bool value);

// 从位图中得到连续的 count 位
int bitmapScan(bitmap_t *map, uint32 count);

#endif