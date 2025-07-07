#ifndef _PAGING_H
#define _PAGING_H

#include <stddef.h>
#include <kernel/memory/mem_entry.h>

void setup_paging(struct MemoryMapEntry *memory_map, uint64_t memory_map_size, uint64_t descriptor_size, uint64_t fb_base, uint64_t fb_size);

#endif
