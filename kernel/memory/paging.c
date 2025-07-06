#include <stdint.h>
#include <kernel/io/print.h>

#define PAGE_PRESENT 0x1
#define PAGE_RW      0x2
#define PAGE_SIZE    0x1000

__attribute__((aligned(0x1000))) static uint64_t pml4[512];
__attribute__((aligned(0x1000))) static uint64_t pdpt[512];
__attribute__((aligned(0x1000))) static uint64_t pd[512];

void setup_paging(void) {
    for (int i = 0; i < 512; ++i) {
        pd[i] = (i * 0x200000) | PAGE_PRESENT | PAGE_RW | (1 << 7); // 2MB pages
    }

    pdpt[0] = ((uint64_t)pd) | PAGE_PRESENT | PAGE_RW;
    pml4[0] = ((uint64_t)pdpt) | PAGE_PRESENT | PAGE_RW;

    uint64_t pml4_phys = (uint64_t)pml4;
    uint64_t cr4_pae = (1 << 5);       // PAE
    uint64_t cr0_pg  = 0x80000000;     // PG

    asm volatile (
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

    kprint("[Paging] Initialized!\n");
}
