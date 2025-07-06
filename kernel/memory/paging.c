#include <stdint.h>
#include <kernel/memory/paging.h>
#include <kernel/io/print.h>

#define PAGE_PRESENT 0x1
#define PAGE_RW      0x2
#define PAGE_PS      (1 << 7) // 2 MiB pages
#define PAGE_SIZE    0x1000
#define MAP_GIB      512      // Identity map 512 GiB

__attribute__((aligned(PAGE_SIZE))) static uint64_t pml4[512];
__attribute__((aligned(PAGE_SIZE))) static uint64_t pdpt[512];
__attribute__((aligned(PAGE_SIZE))) static uint64_t pds[MAP_GIB][512]; // one PD per GiB

void setup_paging(void) {
    // Fill PDs with 2MiB identity-mapped pages
    for (int pdpt_i = 0; pdpt_i < MAP_GIB; ++pdpt_i) {
        for (int i = 0; i < 512; ++i) {
            uint64_t addr = ((uint64_t)pdpt_i * 512 + i) * 0x200000;
            pds[pdpt_i][i] = addr | PAGE_PRESENT | PAGE_RW | PAGE_PS;
        }

        pdpt[pdpt_i] = ((uint64_t)pds[pdpt_i]) | PAGE_PRESENT | PAGE_RW;
    }

    // Identity map PDPT into PML4[0] (used for low memory)
    pml4[0] = ((uint64_t)pdpt) | PAGE_PRESENT | PAGE_RW;

    // High-half kernel mapping (at 0xFFFF800000000000)
    pml4[256] = ((uint64_t)pdpt) | PAGE_PRESENT | PAGE_RW;

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

    kprintf("[Paging] Identity-mapped %d GiB, PML4: 0x%x\n", MAP_GIB, pml4_phys);
}
