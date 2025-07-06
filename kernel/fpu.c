#include <stdint.h>
#include <kernel/io/print.h>

void enable_fpu_sse(void) {
    uint64_t cr0, cr4;

    // Read CR0
    __asm__ volatile ("mov %%cr0, %0" : "=r"(cr0));
    cr0 &= ~(1 << 2); // Clear EM: disable "no FPU"
    cr0 |=  (1 << 1); // Set MP: monitor co-processor
    __asm__ volatile ("mov %0, %%cr0" : : "r"(cr0));

    // Read CR4
    __asm__ volatile ("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= (1 << 9);  // OSFXSR: enable FXSAVE/FXRSTOR
    cr4 |= (1 << 10); // OSXMMEXCPT: unmasked SSE exceptions
    __asm__ volatile ("mov %0, %%cr4" : : "r"(cr4));

    // Initialize the FPU
    __asm__ volatile ("fninit");

    kprintf("[FPU] Initialized! Test: %f\n", 0.123456);
}
