#include <stdint.h>
#include <stddef.h>

#include <xencore/arch/x86_64/paging.h>
#include <xencore/xenio/tty.h>
#include <xencore/common.h>

#define ALIGN_DOWN(x, a) ((uint64_t)(x) & ~((uint64_t)(a) - 1))
#define ALIGN_UP(x, a)   ((((uint64_t)(x) + ((uint64_t)(a) - 1)) & ~((uint64_t)(a) - 1)))
#define ALIGN_DOWN_4K(x) ALIGN_DOWN((x), PAGE_SIZE_4KB)
#define ALIGN_UP_4K(x)   ALIGN_UP((x),   PAGE_SIZE_4KB)
#define ALIGN_DOWN_2M(x) ALIGN_DOWN((x), PAGE_SIZE_2MB)
#define ALIGN_UP_2M(x)   ALIGN_UP((x),   PAGE_SIZE_2MB)

__attribute__((aligned(PAGE_SIZE_4KB))) uint64_t kernel_pml4[512];

uint64_t next_virtual_heap_addr = VIRT_HEAP_BASE;

static uint8_t *early_alloc_buffer = NULL;
static size_t early_alloc_offset = 0;

static void *early_alloc_page(void) {
    if (early_alloc_offset + PAGE_SIZE_4KB > ALLOC_BUFFER * PAGE_SIZE_4KB) {
        tty_printf("[Paging] Early allocation buffer exhausted!\n");
        while (1) { halt(); }
    }
    void *ptr = &early_alloc_buffer[early_alloc_offset];
    early_alloc_offset += PAGE_SIZE_4KB;
    return (void *)ALIGN_UP_4K((uint64_t)ptr);
}

static uint64_t *get_or_create_table(uint64_t *parent, size_t index, uint64_t flags) {
    if (!(parent[index] & PAGE_PRESENT)) {
        uint64_t *new_table = early_alloc_page();
        for (int i = 0; i < 512; ++i) new_table[i] = 0;
        parent[index] = ((uint64_t)new_table) | (flags & (PAGE_RW | PAGE_USER)) | PAGE_PRESENT;
    }
    return (uint64_t *)(parent[index] & ~0xFFFULL);
}

// Map single 4K page
static void map_page_4k(uint64_t *pml4, uint64_t virt, uint64_t phys, uint64_t flags) {
#ifdef HLOS_DEBUG
    if ((virt & (PAGE_SIZE_4KB - 1)) || (phys & (PAGE_SIZE_4KB - 1))) {
        tty_printf("[Paging] WARN: 4K map unaligned v=%p p=%p\n", (void*)virt, (void*)phys);
    }
#endif
    size_t pml4_index = (virt >> 39) & 0x1FF;
    size_t pdpt_index = (virt >> 30) & 0x1FF;
    size_t pd_index   = (virt >> 21) & 0x1FF;
    size_t pt_index   = (virt >> 12) & 0x1FF;

    uint64_t *pdpt = get_or_create_table(pml4, pml4_index, flags);
    uint64_t *pd   = get_or_create_table(pdpt, pdpt_index, flags);
    uint64_t *pt   = get_or_create_table(pd, pd_index, flags);

    pt[pt_index] = phys | (flags & ~(PAGE_PS)) | PAGE_PRESENT;
}

// Map single 2M page
static void map_page_2mb(uint64_t *pml4, uint64_t virt, uint64_t phys, uint64_t flags) {
#ifdef HLOS_DEBUG
    if ((virt & (PAGE_SIZE_2MB - 1)) || (phys & (PAGE_SIZE_2MB - 1))) {
        tty_printf("[Paging] WARN: 2M map unaligned v=%p p=%p\n", (void*)virt, (void*)phys);
    }
#endif
    size_t pml4_index = (virt >> 39) & 0x1FF;
    size_t pdpt_index = (virt >> 30) & 0x1FF;
    size_t pd_index   = (virt >> 21) & 0x1FF;

    uint64_t *pdpt = get_or_create_table(pml4, pml4_index, flags);
    uint64_t *pd   = get_or_create_table(pdpt, pdpt_index, flags);

    pd[pd_index] = phys | (flags | PAGE_PS) | PAGE_PRESENT;
}

// Hybrid mapper: uses 2M pages where possible, 4K for leftovers
static void map_range(uint64_t *pml4, uint64_t virt_start, uint64_t phys_start, uint64_t size, uint64_t flags) {
    uint64_t virt = virt_start;
    uint64_t phys = phys_start;
    uint64_t end  = phys_start + size;

    while (phys + PAGE_SIZE_2MB <= end &&
           !(virt & (PAGE_SIZE_2MB - 1)) &&
           !(phys & (PAGE_SIZE_2MB - 1))) {
        map_page_2mb(pml4, virt, phys, flags);
        virt += PAGE_SIZE_2MB;
        phys += PAGE_SIZE_2MB;
    }

    while (phys < end) {
        map_page_4k(pml4, virt, phys, flags);
        virt += PAGE_SIZE_4KB;
        phys += PAGE_SIZE_4KB;
    }
}

void map_identity(struct MemoryMapEntry *entry) {
    uint64_t phys_start = entry->physical_start;
    uint64_t size_bytes = entry->size_pages * PAGE_SIZE_4KB;

    map_range(kernel_pml4, phys_start, phys_start, size_bytes, PAGE_RW);
#ifdef HLOS_DEBUG
    tty_printf(
        "[Paging] Identity mapped %u KiB @ 0x%x\n",
        entry->size_pages * 4, (uint64_t)phys_start
    );
#endif
}

size_t map_virtual(struct MemoryMapEntry *entry) {
    uint64_t phys_start = entry->physical_start;
    uint64_t virt_start = entry->virtual_start;
    uint64_t size_bytes = entry->size_pages * PAGE_SIZE_4KB;

    map_range(kernel_pml4, virt_start, phys_start, size_bytes, PAGE_RW);
#ifdef HLOS_DEBUG
    tty_printf(
        "[Paging] Mapped %u KiB: virt 0x%x -> phys 0x%x\n",
        entry->size_pages * 4, (uint64_t)virt_start, (uint64_t)phys_start
    );
#endif
    return size_bytes;
}

void setup_paging(struct MemoryMapParams *params, uint64_t fb_base, size_t fb_size)
{
    // Find memory for early allocator
    size_t alloc_size = ALLOC_BUFFER * PAGE_SIZE_4KB;
    size_t alloc_start = 0;
    for (size_t i = 0; i < params->memory_map_size; i += params->descriptor_size) {
        struct MemoryMapEntry *entry = (struct MemoryMapEntry *)((uint8_t *)params->memory_map + i);
        if (entry->type == ConventionalMemory && entry->size_pages * PAGE_SIZE_4KB >= alloc_size) {
            alloc_start = ALIGN_UP_4K(entry->physical_start);
            entry->physical_start += alloc_size;
            entry->size_pages -= alloc_size / PAGE_SIZE_4KB;
            break;
        }
    }

    if (alloc_start == 0) {
        tty_printf("[Paging] No suitable memory found for early allocation buffer!\n");
        while (1) { halt(); }
    }

    early_alloc_buffer = (uint8_t *)alloc_start;
    early_alloc_offset = 0;

    // Identity map framebuffer
    size_t fb_pages = (fb_size / PAGE_SIZE_4KB) + 1;
    struct MemoryMapEntry fb_entry = { 0, 0, fb_base, fb_base, fb_pages, 0 };
    map_identity(&fb_entry);

    // Map all UEFI entries
    for (size_t i = 0; i < params->memory_map_size; i += params->descriptor_size) {
        struct MemoryMapEntry *entry = (struct MemoryMapEntry *)((uint8_t *)params->memory_map + i);
        switch (entry->type) {
            case KernelCode:
            case KernelData:
            case BootServicesCode:
            case BootServicesData:
            case RuntimeServicesCode:
            case RuntimeServicesData:
            case ACPIReclaimMemory:
            case ACPIMemoryNVS:
            case MemoryMappedIO:
            case MemoryMappedIOPortSpace:
            case PalCode:
            case PersistentMemory:
            case UnacceptedMemory:
                map_identity(entry);
                break;

            case ConventionalMemory:
                if (entry->size_pages >= HEAP_MIN_SIZE) {
                    entry->virtual_start = next_virtual_heap_addr;
                    next_virtual_heap_addr += map_virtual(entry);
                }
                break;

            default:
                break;
        }
    }

    // Enable paging
    uint64_t pml4_phys = (uint64_t)kernel_pml4;
    uint64_t cr4_pae = (1 << 5);       // Enable PAE
    uint64_t cr0_pg  = 0x80000000;     // Enable paging

    __asm__ volatile (
        "mov %0, %%cr3\n"
        "mov %%cr4, %%rax\n"
        "or %1, %%rax\n"
        "mov %%rax, %%cr4\n"
        "mov %%cr0, %%rax\n"
        "or %2, %%rax\n"
        "mov %%rax, %%cr0\n"
        :
        : "r"(pml4_phys), "r"(cr4_pae), "r"(cr0_pg)
        : "rax"
    );

    tty_printf("[Paging] Initialized with hybrid mapping!\n");
}
