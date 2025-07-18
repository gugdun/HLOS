#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef ARCH_x86_64
#include <xencore/arch/x86_64/paging.h>
#endif

#include <xencore/xenmem/xenalloc.h>
#include <xencore/xenmem/xenmap.h>
#include <xencore/xenio/tty.h>
#include <xencore/common.h>

/* -------------------------------------------------------------------------- */
/*  Config / Macros                                                           */
/* -------------------------------------------------------------------------- */

#define ALIGN_UP(x,a)   (((x) + ((a)-1)) & ~((a)-1))
#define ALIGN_DOWN(x,a) ((x) & ~((a)-1))

#define XEN_BLOCK_MAGIC 0x584E424Cu  /* 'X','N','B','L' */

#define XEN_BLOCK_LARGE   (1u << 0)  /* backed by dedicated pages */
#define XEN_BLOCK_ALIGNED (1u << 1)  /* user ptr page-aligned (2MiB) */

typedef struct xen_block {
    uint32_t magic;        /* must be XEN_BLOCK_MAGIC */
    uint32_t flags;        /* XEN_BLOCK_* */
    size_t   size;         /* requested size (bytes) */
    size_t   page_count;   /* total 2MiB pages backing allocation (LARGE/ALIGNED) */
    size_t   user_offset;  /* bytes from &block to user ptr */
    struct xen_block *next;/* free-list link (SMALL only) */
} xen_block_t;

/* Small allocations come out of 2MiB arena pages */
typedef struct xen_page {
    struct xen_page *next;
    size_t used;           /* bytes used in data[] */
    uint8_t data[];        /* flexible; allocated as PAGE_SIZE_2MB - header */
} xen_page_t;

/* Global state */
static xen_page_t  *xen_pages      = NULL;
static xen_block_t *xen_free_list  = NULL;

/* Computed at runtime: payload size per arena page */
#define XEN_PAGE_HEADER_SIZE ALIGN_UP(sizeof(xen_page_t), 16)
/* We'll allocate arena pages as raw 2MiB pages and place xen_page_t header at top. */
#define XEN_PAGE_DATA_SIZE (PAGE_SIZE_2MB - XEN_PAGE_HEADER_SIZE)

/* Allocate N 2MiB pages physically contiguous (assumption: alloc_page() returns
 * page-aligned and subsequent calls fill contiguous space *in current early
 * allocator*. If это не гарантируется, надо строить виртуальное непрерывное
 * пространство через map_range() — TODO.)
 */
static void *xen_alloc_pages(size_t pages)
{
    void *base = NULL;
    for (size_t i = 0; i < pages; ++i) {
        void *p = alloc_page();
        if (!p) {
            /* rollback */
            if (base) {
                for (size_t j = 0; j < i; ++j)
                    free_page((void *)((uintptr_t)base + j * PAGE_SIZE_2MB));
            }
            return NULL;
        }
        if (i == 0)
            base = p;
    }
    return base;
}

static void xen_free_pages(void *base, size_t pages)
{
    for (size_t i = 0; i < pages; ++i) {
        free_page((void *)((uintptr_t)base + i * PAGE_SIZE_2MB));
    }
}

/* -------------------------------------------------------------------------- */
/*  Small (arena) allocations                                                 */
/* -------------------------------------------------------------------------- */

static xen_page_t *xen_new_arena_page(void)
{
    uint8_t *raw = (uint8_t *)alloc_page();
    if (!raw) return NULL;

    xen_page_t *pg = (xen_page_t *)raw;
    pg->next = xen_pages;
    pg->used = 0;

    xen_pages = pg;
    return pg;
}

static void *xen_small_alloc(size_t size)
{
    size_t total = sizeof(xen_block_t) + size;

    if (!xen_pages || (xen_pages->used + total > XEN_PAGE_DATA_SIZE)) {
        if (!xen_new_arena_page())
            return NULL;
    }

    uint8_t *payload = ((uint8_t *)xen_pages) + XEN_PAGE_HEADER_SIZE;
    xen_block_t *blk = (xen_block_t *)(payload + xen_pages->used);

    xen_pages->used += total;

    blk->magic       = XEN_BLOCK_MAGIC;
    blk->flags       = 0; /* small */
    blk->size        = size;
    blk->page_count  = 0;
    blk->user_offset = sizeof(xen_block_t);
    blk->next        = NULL;

    void *user_ptr = (void *)(blk + 1);

#ifdef HLOS_DEBUG
    tty_printf("[XenAlloc] Small %u bytes @ 0x%x\n", size, user_ptr);
#endif
    return user_ptr;
}

/* -------------------------------------------------------------------------- */
/*  Large (non-aligned) allocations                                           */
/* -------------------------------------------------------------------------- */

static void *xen_large_alloc(size_t size)
{
    size_t total      = sizeof(xen_block_t) + size;
    size_t page_count = (total + PAGE_SIZE_2MB - 1) / PAGE_SIZE_2MB;

    void *base = xen_alloc_pages(page_count);
    if (!base) return NULL;

    xen_block_t *blk = (xen_block_t *)base;
    blk->magic       = XEN_BLOCK_MAGIC;
    blk->flags       = XEN_BLOCK_LARGE;
    blk->size        = size;
    blk->page_count  = page_count;
    blk->user_offset = sizeof(xen_block_t);
    blk->next        = NULL;

    void *user_ptr = (void *)(blk + 1);

#ifdef HLOS_DEBUG
    tty_printf(
        "[XenAlloc] Large %u bytes (%u pages) @ 0x%x\n",
        size, page_count, user_ptr
    );
#endif
    return user_ptr;
}

/* -------------------------------------------------------------------------- */
/*  Aligned allocations (2MiB boundary)                                       */
/* -------------------------------------------------------------------------- */

void *xen_alloc_aligned(size_t size)
{
    size = ALIGN_UP(size, 8);

    /* Strategy: allocate 1 header page + N data pages.
     * Returned pointer = start of first data page -> guaranteed 2MiB aligned.
     * Header lives in page immediately preceding user memory.
     */
    size_t data_pages = (size + PAGE_SIZE_2MB - 1) / PAGE_SIZE_2MB;
    size_t total_pages = data_pages + 1;  /* header + data */

    void *base = xen_alloc_pages(total_pages);
    if (!base) return NULL;

    xen_block_t *blk = (xen_block_t *)base;
    blk->magic       = XEN_BLOCK_MAGIC;
    blk->flags       = XEN_BLOCK_LARGE | XEN_BLOCK_ALIGNED;
    blk->size        = size;
    blk->page_count  = total_pages;
    blk->user_offset = PAGE_SIZE_2MB;   /* data begins 1 page after header */
    blk->next        = NULL;

    void *user_ptr = (void *)((uintptr_t)base + PAGE_SIZE_2MB);

#ifdef HLOS_DEBUG
    tty_printf(
        "[XenAlloc] Aligned %u bytes (%u+1 pages) @ 0x%x\n",
        size, data_pages, user_ptr
    );
#endif
    return user_ptr;
}

/* -------------------------------------------------------------------------- */
/*  Public allocation entry                                                   */
/* -------------------------------------------------------------------------- */

void *xen_alloc(size_t size)
{
    size = ALIGN_UP(size, 8);

    /* First, check free list */
    xen_block_t **pprev = &xen_free_list;
    xen_block_t *blk = xen_free_list;
    while (blk) {
        if (blk->size >= size) {
            *pprev = blk->next;
            blk->next = NULL;
#ifdef HLOS_DEBUG
            tty_printf("[XenAlloc] Reuse %u bytes @ 0x%x\n", size, (void*)(blk+1));
#endif
            return (void *)(blk + 1);
        }
        pprev = &blk->next;
        blk = blk->next;
    }

    /* Decide path */
    if (size + sizeof(xen_block_t) > XEN_PAGE_DATA_SIZE) {
        return xen_large_alloc(size);
    } else {
        return xen_small_alloc(size);
    }
}

/* -------------------------------------------------------------------------- */
/*  Free                                                                      */
/* -------------------------------------------------------------------------- */

void xen_free(void *ptr)
{
    if (!ptr) return;

    uintptr_t up = (uintptr_t)ptr;

    /* First candidate: small/large header immediately before user data */
    xen_block_t *blk = (xen_block_t *)(up - sizeof(xen_block_t));
    if (blk->magic == XEN_BLOCK_MAGIC && blk->user_offset == sizeof(xen_block_t)) {
        if (blk->flags & XEN_BLOCK_LARGE) {
            /* came from xen_large_alloc() */
            xen_free_pages((void *)blk, blk->page_count);
        } else {
            /* small -> push to freelist */
            blk->next = xen_free_list;
            xen_free_list = blk;
        }
#ifdef HLOS_DEBUG
        tty_printf(
            "[XenAlloc] Free %s block @ 0x%x\n",
            (blk->flags & XEN_BLOCK_LARGE) ? "large" : "small",
            ptr
        );
#endif
        return;
    }

    /* Second candidate: aligned allocation header one page below */
    blk = (xen_block_t *)(up - PAGE_SIZE_2MB);
    if (blk->magic == XEN_BLOCK_MAGIC &&
        (blk->flags & XEN_BLOCK_ALIGNED) &&
        blk->user_offset == PAGE_SIZE_2MB)
    {
#ifdef HLOS_DEBUG
        tty_printf(
            "[XenAlloc] Free aligned block @ 0x%x (%u pages)\n",
            ptr, blk->page_count - 1
        );
#endif
        xen_free_pages((void *)blk, blk->page_count);
        return;
    }

    /* If we got here — corrupted pointer */
    tty_printf("[XenAlloc] ERROR: invalid free 0x%x\n", ptr);
    halt(); /* or ignore */
}
