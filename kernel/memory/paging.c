#include <stdint.h>
#include <kernel/memory/paging.h>
#include <kernel/io/print.h>

#define PAGE_PRESENT 0x1
#define PAGE_RW      0x2
#define PAGE_PS      (1 << 7) // 2 MiB pages
#define PAGE_SIZE    0x1000
#define MAP_GIB      4096

__attribute__((aligned(PAGE_SIZE))) static uint64_t pml4[512];
__attribute__((aligned(PAGE_SIZE))) static uint64_t pdpt[512];
__attribute__((aligned(PAGE_SIZE))) static uint64_t pds[MAP_GIB][512]; // one PD per GiB

static void map_identity(struct MemoryMapEntry *entry)
{
    uint64_t start = entry->physical_start - (entry->physical_start % 0x200000);
    uint64_t pages = (entry->size_pages / (uint64_t)512) + 1;
    for (uint64_t i = 0; i < pages; ++i) {
        uint64_t addr = start + i * 0x200000;
        pds[(addr >> 30) & 0x1FF][(addr >> 21) & 0x1FF] = addr | PAGE_PRESENT | PAGE_RW | PAGE_PS;
        pdpt[(addr >> 30) & 0x1FF] = ((uint64_t)pds[(addr >> 30) & 0x1FF]) | PAGE_PRESENT | PAGE_RW;
    }
    kprintf("[Paging] Identity mapped %u pages at 0x%x (type %d)\n", pages, start, entry->type);
}

void setup_paging(struct MemoryMapEntry *memory_map, uint64_t memory_map_size, uint64_t descriptor_size, uint64_t fb_base, uint64_t fb_size)
{
    // Identity map framebuffer
    uint64_t fb_pages = (fb_size / (uint64_t)4096) + 1;
    struct MemoryMapEntry fb_entry = { 0, 0, fb_base, fb_base, fb_pages, 0 };
    map_identity(&fb_entry);

    // Identity map entries
    for (uint64_t i = 0; i < memory_map_size; i += descriptor_size) {
        struct MemoryMapEntry *entry = (struct MemoryMapEntry *)((uint8_t *)memory_map + i);
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
            default:
                break;
        }
    }

    // Identity map PDPT into PML4[0] (used for low memory)
    pml4[0] = ((uint64_t)pdpt) | PAGE_PRESENT | PAGE_RW;

    // Enable paging
    uint64_t pml4_phys = (uint64_t)pml4;
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

    kprintf("[Paging] Initialized!\n");
}
