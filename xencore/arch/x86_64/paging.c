#include <stdint.h>

#include <xencore/arch/x86_64/paging.h>

#include <xencore/xenio/tty.h>
#include <xencore/common.h>

#define PAGE_PRESENT  0x1
#define PAGE_RW       0x2
#define PAGE_PS       (1 << 7)  // Use 2 MiB pages
#define HEAP_MIN_SIZE 512       // In 4 KiB pages
#define ALLOC_BUFFER  512       // In 4 KiB pages

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
    return ptr;
}

static uint64_t *get_or_create_table(uint64_t *parent, size_t index) {
    if (!(parent[index] & PAGE_PRESENT)) {
        uint64_t *new_table = early_alloc_page();
        for (int i = 0; i < 512; ++i) new_table[i] = 0;
        parent[index] = ((uint64_t)new_table) | PAGE_PRESENT | PAGE_RW;
    }
    return (uint64_t *)(parent[index] & ~0xFFFULL);
}

static void map_page_2mb(uint64_t virt, uint64_t phys) {
    size_t pml4_index = (virt >> 39) & 0x1FF;
    size_t pdpt_index = (virt >> 30) & 0x1FF;
    size_t pd_index   = (virt >> 21) & 0x1FF;

    uint64_t *pdpt = get_or_create_table(kernel_pml4, pml4_index);
    uint64_t *pd   = get_or_create_table(pdpt, pdpt_index);

    if (!(pd[pd_index] & PAGE_PRESENT)) {
        pd[pd_index] = phys | PAGE_PRESENT | PAGE_RW | PAGE_PS;
    }
}

void map_identity(struct MemoryMapEntry *entry) {
    uint64_t start = entry->physical_start & ~(PAGE_SIZE_2MB - 1);
    uint64_t end   = entry->physical_start + (entry->size_pages * PAGE_SIZE_4KB);

    for (uint64_t addr = start; addr < end; addr += PAGE_SIZE_2MB) {
        map_page_2mb(addr, addr);
    }

#ifdef HLOS_DEBUG
    tty_printf(
        "[Paging] Identity mapped %u KiB @ 0x%x\n",
        entry->size_pages * 4, entry->physical_start
    );
#endif
}

size_t map_virtual(struct MemoryMapEntry *entry) {
    uint64_t phys_start = entry->physical_start & ~(PAGE_SIZE_2MB - 1);
    uint64_t virt_start = entry->virtual_start & ~(PAGE_SIZE_2MB - 1);
    uint64_t end = entry->physical_start + (entry->size_pages * PAGE_SIZE_4KB);

    for (uint64_t offset = 0; phys_start + offset < end; offset += PAGE_SIZE_2MB) {
        map_page_2mb(virt_start + offset, phys_start + offset);
    }

#ifdef HLOS_DEBUG
    tty_printf(
        "[Paging] Mapped %u KiB: virt 0x%x -> phys 0x%x\n",
        entry->size_pages * 4, virt_start, phys_start
    );
#endif

    return end - phys_start;
}

void setup_paging(struct MemoryMapParams *params, uint64_t fb_base, size_t fb_size)
{
    // Find conventional memory for alloc buffer
    size_t alloc_size = ALLOC_BUFFER * PAGE_SIZE_4KB;
    size_t alloc_start = 0;
    for (size_t i = 0; i < params->memory_map_size; i += params->descriptor_size) {
        struct MemoryMapEntry *entry = (struct MemoryMapEntry *)((uint8_t *)params->memory_map + i);
        if (entry->type == ConventionalMemory && entry->size_pages * PAGE_SIZE_4KB >= alloc_size) {
            alloc_start = entry->physical_start;
            entry->physical_start += alloc_size; // Reserve the space
            entry->size_pages -= alloc_size / PAGE_SIZE_4KB; // Reduce the size
            break;
        }
    }

    if (alloc_start == 0) {
        tty_printf("[Paging] No suitable memory found for early allocation buffer!\n");
        while (1) { halt(); }
    }

    early_alloc_buffer = (uint8_t *)alloc_start;
    early_alloc_offset = 0;

    // Map framebuffer
    size_t fb_pages = (fb_size / (uint64_t)PAGE_SIZE_4KB) + 1;
    struct MemoryMapEntry fb_entry = { 0, 0, fb_base, fb_base, (uint64_t)fb_pages, 0 };
    map_identity(&fb_entry);

    // Map UEFI memory map entries
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

    tty_printf("[Paging] Initialized!\n");
}
