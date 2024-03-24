#ifndef SOUL_ARENA_H
#define SOUL_ARENA_H

#include <types/types.h>
#include <lib/list.h>

#define DESC_COUNT 7

typedef list_node_t block_t; // 内存块

// 内存描述符
typedef struct arena_descriptor_t
{
    uint32 totalBlock;  // 一页内存分成了多少块
    uint32 blockSize;   // 块大小
    list_t freeList; // 空闲列表
} arena_descriptor_t;

// 一页或多页内存
typedef struct arena_t
{
    arena_descriptor_t *desc; // 该 arena 的描述符
    uint32 count;                // 当前剩余多少块 或 页数
    uint32 large;                // 表示是不是超过 1024 字节
    uint32 magic;                // 魔数
} arena_t;

void *kmalloc(size_t size);
void kfree(void *ptr);

#endif