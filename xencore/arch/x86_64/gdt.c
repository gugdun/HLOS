#include <xencore/arch/x86_64/gdt.h>
#include <xencore/arch/x86_64/tss.h>

#include <xencore/io/tty.h>

extern struct TSS tss;

__attribute__((aligned(16))) struct GDTEntry gdt[GDT_ENTRIES];
struct GDTPtr gdt_ptr;

void setup_gdt() {
    // NULL
    gdt[0] = (struct GDTEntry){0};

    // Kernel Code Segment (0x08)
    gdt[1] = (struct GDTEntry){
        .limit_low = 0,
        .base_low = 0,
        .base_mid = 0,
        .access = 0x9A,       // code, ring 0
        .granularity = 0x20,  // 64-bit
        .base_high = 0
    };

    // Kernel Data Segment (0x10)
    gdt[2] = (struct GDTEntry){
        .limit_low = 0,
        .base_low = 0,
        .base_mid = 0,
        .access = 0x92,      // data, ring 0
        .granularity = 0,
        .base_high = 0
    };

    // TSS descriptor (low 8 bytes)
    uint64_t base = (uint64_t)&tss;
    uint32_t limit = sizeof(tss) - 1;
    gdt[3].limit_low = limit & 0xFFFF;
    gdt[3].base_low = base & 0xFFFF;
    gdt[3].base_mid = (base >> 16) & 0xFF;
    gdt[3].access = 0x89; // type 9 = available 64-bit TSS
    gdt[3].granularity = ((limit >> 16) & 0x0F);
    gdt[3].base_high = (base >> 24) & 0xFF;

    // TSS descriptor (high 8 bytes)
    *((uint32_t*)&gdt[4]) = (base >> 32) & 0xFFFFFFFF;
    *((uint32_t*)&gdt[4] + 1) = 0;

    gdt_ptr.limit = sizeof(gdt) - 1;
    gdt_ptr.base = (uint64_t)&gdt;

    __asm__ volatile ("lgdt %0" : : "m"(gdt_ptr));

    // Reload segments
    __asm__ volatile (
        "mov $0x10, %%ax\n"
        "mov %%ax, %%ds\n"
        "mov %%ax, %%ss\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"
        "pushq $0x08\n"
        "lea 1f(%%rip), %%rax\n"
        "pushq %%rax\n"
        "lretq\n"
        "1:\n"
        ::: "rax", "memory"
    );

    __asm__ volatile ("ltr %w0" : : "r"((uint16_t)0x18));

    tty_printf("[GDT] Base: 0x%x\n", (uint64_t)&gdt);
}
