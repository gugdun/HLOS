#include <stdint.h>

#include <xencore/arch/x86_64/paging.h>

#include <xencore/xenio/tty.h>

#define PAGE_PRESENT  0x1
#define PAGE_RW       0x2
#define PAGE_PS       (1 << 7)  // Use 2 MiB pages
#define HEAP_MIN_SIZE 512       // In 4 KiB pages

__attribute__((aligned(PAGE_SIZE_4KB))) uint64_t kernel_pml4[512];
__attribute__((aligned(PAGE_SIZE_4KB))) uint64_t kernel_pdpt_low[512];
__attribute__((aligned(PAGE_SIZE_4KB))) uint64_t kernel_pds_low[PAGING_MAP_GIB][512];
__attribute__((aligned(PAGE_SIZE_4KB))) uint64_t kernel_pdpt_high[512];
__attribute__((aligned(PAGE_SIZE_4KB))) uint64_t kernel_pds_high[PAGING_MAP_GIB][512];

uint64_t next_virtual_heap_addr = VIRT_HEAP_BASE;

void map_identity(struct MemoryMapEntry *entry)
{
    uint64_t start = entry->physical_start - (entry->physical_start % PAGE_SIZE_2MB);
    size_t   pages = (entry->size_pages / (uint64_t)(PAGE_SIZE_2MB / PAGE_SIZE_4KB)) + 1;

    for (uint64_t i = 0; i < pages; ++i) {
        uint64_t addr = start + i * PAGE_SIZE_2MB;

        uint64_t pd_index   = (addr >> 21) & 0x1FF;
        uint64_t pdpt_index = (addr >> 30) & 0x1FF;
        uint64_t pml4_index = (addr >> 39) & 0x1FF;

        kernel_pds_low[pdpt_index][pd_index] = addr | PAGE_PRESENT | PAGE_RW | PAGE_PS;
        kernel_pdpt_low[pdpt_index] = ((uint64_t)kernel_pds_low[pdpt_index]) | PAGE_PRESENT | PAGE_RW;
        kernel_pml4[pml4_index] = ((uint64_t)kernel_pdpt_low) | PAGE_PRESENT | PAGE_RW;
    }
}

size_t map_virtual(struct MemoryMapEntry *entry, uint64_t pml4[512], uint64_t pdpt[512], uint64_t pds[PAGING_MAP_GIB][512])
{
    const uint64_t phys_mod     = entry->physical_start % PAGE_SIZE_2MB;
    const uint64_t phys_start   = entry->physical_start - phys_mod;
    const uint64_t virt_mod     = entry->virtual_start % PAGE_SIZE_2MB;
    const uint64_t virt_start   = entry->virtual_start - virt_mod;
    const size_t   size_bytes   = entry->size_pages * PAGE_SIZE_4KB + phys_mod;
    const size_t   aligned_size = (size_bytes + PAGE_SIZE_2MB - 1) / PAGE_SIZE_2MB * PAGE_SIZE_2MB;

    for (size_t offset = 0; offset < aligned_size; offset += PAGE_SIZE_2MB) {
        uint64_t phys = phys_start + offset;
        uint64_t virt = virt_start + offset;
        
        uint64_t pd_index   = (virt >> 21) & 0x1FF;
        uint64_t pdpt_index = (virt >> 30) & 0x1FF;
        uint64_t pml4_index = (virt >> 39) & 0x1FF;

        pds[pdpt_index][pd_index] = phys | PAGE_PRESENT | PAGE_RW | PAGE_PS;
        pdpt[pdpt_index] = ((uint64_t)pds[pdpt_index]) | PAGE_PRESENT | PAGE_RW;
        pml4[pml4_index] = ((uint64_t)pdpt) | PAGE_PRESENT | PAGE_RW;
    }

#ifdef HLOS_DEBUG
    tty_printf("[Paging] Mapped %u bytes at virt 0x%x -> phys 0x%x\n", aligned_size, virt_start, phys_start);
#endif

    return aligned_size;
}

void setup_paging(struct MemoryMapParams *params, uint64_t fb_base, size_t fb_size)
{
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
                    next_virtual_heap_addr += map_virtual(entry, kernel_pml4, kernel_pdpt_high, kernel_pds_high);
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
