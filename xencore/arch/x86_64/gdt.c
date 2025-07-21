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

    // Lower 64 bits of TSS descriptor
    gdt[idx].limit_low    = limit & 0xFFFF;
    gdt[idx].base_low     = base & 0xFFFF;
    gdt[idx].base_mid     = (base >> 16) & 0xFF;
    gdt[idx].access       = 0x89; // present=1, type=1001b (64-bit TSS)
    gdt[idx].granularity  = (limit >> 16) & 0x0F;
    gdt[idx].base_high    = (base >> 24) & 0xFF;

    // Upper 64 bits
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
        .limit_low   = 0,
        .base_low    = 0,
        .base_mid    = 0,
        .access      = 0x9A,   // Code: present=1, DPL=0, code, readable
        .granularity = 0x20,   // L=1 (64-bit), D=0, G=0
        .base_high   = 0
    };

    // ---- Kernel Data (0x10) ----
    gdt[GDT_KERNEL_DATA] = (struct GDTEntry){
        .limit_low   = 0,
        .base_low    = 0,
        .base_mid    = 0,
        .access      = 0x92,   // Data: present=1, DPL=0, writable
        .granularity = 0x00,   // No flags needed
        .base_high   = 0
    };

    // ---- User Code (0x18 base) ----
    gdt[GDT_USER_CODE] = (struct GDTEntry){
        .limit_low   = 0,
        .base_low    = 0,
        .base_mid    = 0,
        .access      = 0xFA,   // User code: present=1, DPL=3
        .granularity = 0x20,   // L=1
        .base_high   = 0
    };

    // ---- User Data (0x20 base) ----
    gdt[GDT_USER_DATA] = (struct GDTEntry){
        .limit_low   = 0,
        .base_low    = 0,
        .base_mid    = 0,
        .access      = 0xF2,   // User data: present=1, DPL=3, writable
        .granularity = 0x00,
        .base_high   = 0
    };

    // ---- TSS descriptor (0x28) ----
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

    tty_printf(
        "[GDT] Initialized. Base=0x%x, limit=%u\n",
        (uint64_t)&gdt, gdt_ptr.limit
    );
}
