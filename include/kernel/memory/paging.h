#ifndef _PAGING_H
#define _PAGING_H

#include <stddef.h>
#include <kernel/memory/mem_entry.h>

#define PAGING_MAP_GIB 512
#define PAGE_SIZE_4KB  0x1000
#define PAGE_SIZE_2MB  0x200000
#define VIRT_HEAP_BASE 0xFFFF800000000000ULL
#define MAX_PAGES      (PAGING_MAP_GIB * 512)

void setup_paging(struct MemoryMapEntry *memory_map, size_t memory_map_size, size_t descriptor_size, uint64_t fb_base, size_t fb_size);

#endif
