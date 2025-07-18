#ifndef _PAGING_H
#define _PAGING_H

#include <stdint.h>
#include <stddef.h>

#include <xencore/xenmem/mem_entry.h>

#define PAGE_SIZE_4KB   0x1000
#define PAGE_SIZE_2MB   0x200000
#define VIRT_HEAP_BASE  0xFFFF800000000000ULL
#define HEAP_MIN_SIZE   1024    // In 4 KiB pages
#define ALLOC_BUFFER    1024    // Number of 4KB pages for early allocation

#define PAGE_PRESENT  (1ULL << 0)
#define PAGE_RW       (1ULL << 1)
#define PAGE_USER     (1ULL << 2)
#define PAGE_PS       (1ULL << 7)
#define PAGE_NX       (1ULL << 63)

struct MemoryMapParams {
    struct MemoryMapEntry *memory_map;
    size_t memory_map_size;
    size_t descriptor_size;
};

uint64_t virt_to_phys(uint64_t virt);
void *early_alloc_page(void);
void load_pml4(uint64_t *pml4);
uint64_t *create_user_pml4(void);
void map_range(uint64_t *pml4, uint64_t virt_start, uint64_t phys_start, uint64_t size, uint64_t flags);
void map_user_segment(uint64_t *user_pml4, uint64_t virt, uint64_t phys, uint64_t size);
void map_identity(struct MemoryMapEntry *entry);
size_t map_virtual(struct MemoryMapEntry *entry);
void setup_paging(struct MemoryMapParams *params, uint64_t fb_base, size_t fb_size);

#endif
