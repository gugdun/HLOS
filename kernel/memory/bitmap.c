#include <kernel/memory/bitmap.h>
#include <kernel/memory/paging.h>
#include <string.h>

extern uint64_t next_virtual_heap_addr;

static uint64_t page_bitmap[MAX_PAGES / 64]; // 1 bit per page
static uint64_t   total_pages = 0;

void bitmap_init()
{
    total_pages = (next_virtual_heap_addr - VIRT_HEAP_BASE) / PAGE_SIZE_2MB;
    memset((void *)page_bitmap, 0, MAX_PAGES / 8);
}

void *alloc_page()
{
    for (uint64_t i = 0; i < total_pages; ++i) {
        if (!(page_bitmap[i / 64] & (1 << (i % 64)))) {
            page_bitmap[i / 64] |= (1 << (i % 64)); // mark as used
            uint64_t addr = VIRT_HEAP_BASE + i * PAGE_SIZE_2MB;
            return (void *)addr;
        }
    }
    return NULL; // out of memory
}

void free_page(void *page)
{
    uint64_t page_index = ((uint64_t)page - VIRT_HEAP_BASE) / PAGE_SIZE_2MB;
    page_bitmap[page_index / 64] &= ~(1 << (page_index % 64)); // mark as free
}
