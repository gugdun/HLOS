#include <string.h>

#include <xencore/arch/x86_64/gdt.h>
#include <xencore/arch/x86_64/tss.h>
#include <xencore/arch/x86_64/segments.h>

#include <xencore/xenio/tty.h>

extern struct TSS tss;

__attribute__((aligned(16))) struct GDTEntry gdt[GDT_ENTRIES];
struct GDTPtr gdt_ptr;

static inline void write_tss_descriptor(int idx, struct TSS* tss)
{
    uint64_t base  = (uint64_t)tss;
    uint32_t limit = sizeof(struct TSS) - 1;

    // Lower (GDTEntry)
    gdt[idx].limit_low  = limit & 0xFFFF;
    gdt[idx].base_low   = base & 0xFFFF;
    gdt[idx].base_mid   = (base >> 16) & 0xFF;
    gdt[idx].access     = 0x89; // present=1, DPL=0, type=1001b (available 64-bit TSS)
    gdt[idx].granularity = ((limit >> 16) & 0x0F);
    gdt[idx].granularity |= 0;  // G=0, AVL=0, L=0, D/B=0 for system segment
    gdt[idx].base_high  = (base >> 24) & 0xFF;

    // Upper (TSSDescriptorHigh)
    struct TSSDescriptorHigh* high = (struct TSSDescriptorHigh*)&gdt[idx + 1];
    high->base_upper = (base >> 32) & 0xFFFFFFFF;
    high->reserved   = 0;
}

void setup_gdt(void)
{
    memset(gdt, 0, sizeof(gdt));

    // ---- Null ----
    // gdt[0] = null

    // ---- Kernel Code (0x08) ----
    gdt[GDT_KERNEL_CODE] = (struct GDTEntry){
        .limit_low   = 0x0000,
        .base_low    = 0x0000,
        .base_mid    = 0x00,
        .access      = 0x9A,   // present=1, DPL=0, code seg, executable, readable
        .granularity = 0x20,   // L=1 (64-bit), G=0, D=0
        .base_high   = 0x00
    };

    // ---- Kernel Data (0x10) ----
    gdt[GDT_KERNEL_DATA] = (struct GDTEntry){
        .limit_low   = 0x0000,
        .base_low    = 0x0000,
        .base_mid    = 0x00,
        .access      = 0x92,   // present=1, DPL=0, data seg, writable
        .granularity = 0x00,   // L=0 для data; D=0; G=0
        .base_high   = 0x00
    };

    // ---- User Code (0x18 base, selector=0x1B) ----
    gdt[GDT_USER_CODE] = (struct GDTEntry){
        .limit_low   = 0x0000,
        .base_low    = 0x0000,
        .base_mid    = 0x00,
        .access      = 0xFA,   // present=1, DPL=3, code, executable, readable
        .granularity = 0x20,   // L=1
        .base_high   = 0x00
    };

    // ---- User Data (0x20 base, selector=0x23) ----
    gdt[GDT_USER_DATA] = (struct GDTEntry){
        .limit_low   = 0x0000,
        .base_low    = 0x0000,
        .base_mid    = 0x00,
        .access      = 0xF2,   // present=1, DPL=3, data, writable
        .granularity = 0x00,
        .base_high   = 0x00
    };

    // ---- TSS descriptor ----
    write_tss_descriptor(GDT_TSS_LOW, &tss);

    // ---- GDT pointer ----
    gdt_ptr.limit = sizeof(gdt) - 1;
    gdt_ptr.base  = (uint64_t)&gdt;

    // ---- Load GDT ----
    __asm__ volatile ("lgdt %0" : : "m"(gdt_ptr));

    // ---- Reload segments ----
    __asm__ volatile (
        "mov %[ds], %%ax\n\t"
        "mov %%ax, %%ds\n\t"
        "mov %%ax, %%es\n\t"
        "mov %%ax, %%ss\n\t"
        "mov %%ax, %%fs\n\t"
        "mov %%ax, %%gs\n\t"
        // Far jump (flush pipeline & load CS)
        "pushq %[cs]\n\t"
        "lea 1f(%%rip), %%rax\n\t"
        "pushq %%rax\n\t"
        "lretq\n\t"
        "1:\n\t"
        :
        : [cs]"i"(KERNEL_CS), [ds]"i"(KERNEL_DS)
        : "rax", "memory"
    );

    // ---- Load TSS ----
    __asm__ volatile ("ltr %w0" : : "r"((uint16_t)TSS_SELECTOR));

    tty_printf("[GDT] Initialized. Base=0x%x, limit=%u\n", (uint64_t)&gdt, gdt_ptr.limit);
}
