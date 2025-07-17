#include <stdbool.h>

#ifdef ARCH_x86_64
#include <xencore/arch/x86_64/paging.h>
#endif

#include <xencore/xenmem/xenalloc.h>
#include <xencore/xenmem/xenmap.h>
#include <xencore/xenio/tty.h>

typedef struct xen_block {
    size_t size;
    struct xen_block *next;
    bool is_large;
} xen_block_t;

typedef struct xen_page {
    struct xen_page *next;
    size_t used;
    uint8_t data[PAGE_SIZE_2MB - sizeof(struct xen_page *) - sizeof(size_t)];
} xen_page_t;

static xen_page_t *xen_pages = (xen_page_t *)NULL;
static xen_block_t *xen_free_list = (xen_block_t *)NULL;

void *xen_alloc_aligned(size_t size)
{
    // Align size to 8 bytes
    size = (size + 7) & ~7;
    size_t total = size + sizeof(xen_block_t);

    // Always allocate full 2 MiB pages (or more)
    size_t pages_needed = (total + PAGE_SIZE_2MB - 1) / PAGE_SIZE_2MB;
    void *aligned_mem = NULL;

    for (size_t i = 0; i < pages_needed; ++i) {
        void *page = alloc_page();
        if (!page) {
            // Rollback allocated pages
            for (size_t j = 0; j < i; ++j)
                free_page((void *)((uintptr_t)aligned_mem + j * PAGE_SIZE_2MB));
            return NULL;
        }

        if (i == 0)
            aligned_mem = page;
    }

    xen_block_t *block = (xen_block_t *)aligned_mem;
    block->size = size;
    block->is_large = true;

#ifdef HLOS_DEBUG
    tty_printf("[XenAlloc] Allocated aligned block of %u bytes @ 0x%x (aligned to 2MiB)\n",
               size, (void *)(block + 1));
#endif
    return (void *)(block + 1);
}

void *xen_alloc(size_t size)
{
    // Align to 8 bytes
    size = (size + 7) & ~7;
    size_t total = size + sizeof(xen_block_t);

    // Check free list
    xen_block_t **prev = &xen_free_list;
    for (xen_block_t *block = xen_free_list; block; block = block->next) {
        if (block->size >= size) {
            *prev = block->next;
            return (void *)(block + 1);
        }
        prev = &block->next;
    }

    // LARGE ALLOCATION: bigger than bump space
    if (total > sizeof(xen_pages->data)) {
        size_t pages_needed = (total + PAGE_SIZE_2MB - 1) / PAGE_SIZE_2MB;
        void *big = NULL;

        for (size_t i = 0; i < pages_needed; ++i) {
            void *page = alloc_page();
            if (!page) {
                for (size_t j = 0; j < i; ++j)
                    free_page((void *)((uintptr_t)big + j * PAGE_SIZE_2MB));
                return NULL;
            }

            if (i == 0)
                big = page;
        }

        xen_block_t *block = (xen_block_t *)big;
        block->size = size;
        block->is_large = true;

#ifdef HLOS_DEBUG
        tty_printf("[XenAlloc] Allocated large block of %u bytes @ 0x%x\n", size, (void *)(block + 1));
#endif
        return (void *)(block + 1);
    }

    // Bump allocation
    if (!xen_pages || (xen_pages->used + total > sizeof(xen_pages->data))) {
        xen_page_t *new_page = (xen_page_t *)alloc_page();
        if (!new_page) return NULL;
        new_page->next = xen_pages;
        new_page->used = 0;
        xen_pages = new_page;
    }

    xen_block_t *block = (xen_block_t *)&xen_pages->data[xen_pages->used];
    block->size = size;
    block->is_large = false;
    xen_pages->used += total;

#ifdef HLOS_DEBUG
    tty_printf("[XenAlloc] Allocated %u bytes @ 0x%x\n", size, (void *)(block + 1));
#endif
    return (void *)(block + 1);
}

void xen_free(void *ptr)
{
    if (!ptr) return;

#ifdef HLOS_DEBUG
    tty_printf("[XenAlloc] Freeing memory @ 0x%x\n", ptr);
#endif

    xen_block_t *block = (xen_block_t *)ptr - 1;

    if (block->is_large) {
        size_t total = block->size + sizeof(xen_block_t);
        size_t pages = (total + PAGE_SIZE_2MB - 1) / PAGE_SIZE_2MB;
        for (size_t i = 0; i < pages; ++i) {
            free_page((void *)((uintptr_t)block + i * PAGE_SIZE_2MB));
        }
        return;
    }

    block->next = xen_free_list;
    xen_free_list = block;
}
