#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/interrupts.h>
#include <kernel/io/serial.h>
#include <kernel/io/print.h>

void kernel_main() {
    serial_init();
    disable_interrupts();
    setup_tss();
    setup_gdt();
    setup_idt();
    enable_interrupts();

    // Trigger a #DE
    volatile int zero = 0;
    volatile int crash = 1 / zero;

    kprint("[Kernel] Halting...\n");
    while (1) asm volatile ("hlt");
}
