#ifndef _PAGING_H
#define _PAGING_H

#include <stddef.h>

#include <xencore/xenmem/mem_entry.h>

#define PAGE_SIZE_4KB  0x1000
#define PAGE_SIZE_2MB  0x200000
#define VIRT_HEAP_BASE 0xFFFF800000000000ULL

struct MemoryMapParams {
    struct MemoryMapEntry *memory_map;
    size_t memory_map_size;
    size_t descriptor_size;
};

static inline void load_pml4(uint64_t pml4_phys)
{
    __asm__ volatile (
        "mov %0, %%cr3\n"
        :
        : "r"(pml4_phys)
        : "memory"
    );
}

void map_identity(struct MemoryMapEntry *entry);
size_t map_virtual(struct MemoryMapEntry *entry);
void setup_paging(struct MemoryMapParams *params, uint64_t fb_base, size_t fb_size);

#endif
