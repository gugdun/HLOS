#include <kernel/gdt.h>
#include <kernel/tss.h>
#include <kernel/io/print.h>

__attribute__((aligned(16))) struct GDTEntry gdt[GDT_ENTRIES];
__attribute__((aligned(16))) struct TSSDescriptorHigh tss_high;
__attribute__((aligned(16))) struct TSS tss;

__attribute__((aligned(16))) uint8_t ist_stack[4096];
__attribute__((aligned(16))) uint8_t df_stack[4096]; // double fault IST

struct GDTPtr gdt_ptr;

void setup_tss() {
    tss.rsp0 = (uint64_t)(ist_stack + sizeof(ist_stack));
    tss.ist[0] = (uint64_t)(df_stack + sizeof(df_stack)); // IST1 = double fault
    tss.iopb_offset = sizeof(struct TSS);
    kprintf("[TSS] Base: 0x%x\n", (uint64_t)&tss);
}

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

    kprintf("[GDT] Base: 0x%x\n", (uint64_t)&gdt);
}
