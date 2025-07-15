#include <string.h>

#ifdef ARCH_x86_64
#include <xencore/arch/x86_64/paging.h>
#endif

#include <xencore/xenmem/xenmap.h>
#include <xencore/xenio/tty.h>

extern uint64_t next_virtual_heap_addr;

static uint64_t page_xenmap[MAX_PAGES / 64]; // 1 bit per page
static size_t   total_pages = 0;

void xenmap_init()
{
    total_pages = (next_virtual_heap_addr - VIRT_HEAP_BASE) / PAGE_SIZE_2MB;
    memset((void *)page_xenmap, 0, MAX_PAGES / 8);
    tty_printf("[Xenmap] Total pages: %u\n", total_pages);
}

void *alloc_page()
{
    for (size_t i = 0; i < total_pages; ++i) {
        if (!(page_xenmap[i / 64] & (1 << (i % 64)))) {
            page_xenmap[i / 64] |= (1 << (i % 64)); // mark as used
            uint64_t addr = VIRT_HEAP_BASE + i * PAGE_SIZE_2MB;
#ifdef HLOS_DEBUG
            tty_printf("[Xenmap] Allocated page @ 0x%x (page index %u)\n", addr, i);
#endif
            return (void *)addr;
        }
    }
    return NULL; // out of memory
}

void free_page(void *page)
{
    uint64_t page_index = ((uint64_t)page - VIRT_HEAP_BASE) / PAGE_SIZE_2MB;
    page_xenmap[page_index / 64] &= ~(1 << (page_index % 64)); // mark as free
#ifdef HLOS_DEBUG
    tty_printf("[Xenmap] Freed page @ 0x%x (page index %u)\n", (uint64_t)page, page_index);
#endif
}
