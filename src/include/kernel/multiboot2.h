#ifndef SOUL_MULTIBOOT_H
#define SOUL_MULTIBOOT_H

#include <types/types.h>

// 进入内核时 eax 寄存器的值
#define MULTIBOOT2_MAGIC 0x36d76289

#define MULTIBOOT_TAG_TYPE_END 0
#define MULTIBOOT_TAG_TYPE_MMAP 6

#define MULTIBOOT_MEMORY_AVAILABLE 1
#define MULTIBOOT_MEMORY_RESERVED 2
#define MULTIBOOT_MEMORY_ACPI_RECLAIMABLE 3
#define MULTIBOOT_MEMORY_NVS 4
#define MULTIBOOT_MEMORY_BADRAM 5

// multiboot tag
typedef struct multi_tag_t
{
    uint32 type;
    uint32 size;
} multi_tag_t;

// multiboot mmap entry
typedef struct multi_mmap_entry_t
{
    uint64 addr;
    uint64 len;
    uint32 type;
    uint32 zero;
} multi_mmap_entry_t;

// multiboot mmap tag
typedef struct multi_tag_mmap_t
{
    uint32 type;
    uint32 size;
    uint32 entry_size;
    uint32 entry_version;
    multi_mmap_entry_t entries[0];
} multi_tag_mmap_t;

#endif