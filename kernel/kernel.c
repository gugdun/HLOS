#include <kernel/fpu.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/interrupts.h>
#include <kernel/io/serial.h>
#include <kernel/io/print.h>
#include <kernel/memory/paging.h>
#include <kernel/graphics/framebuffer.h>

void kernel_main(uint64_t fb_base, uint64_t fb_size, uint32_t fb_width, uint32_t fb_height) {
    serial_init();
    enable_fpu_sse();
    fb_init(fb_base, fb_size, fb_width, fb_height);
    disable_interrupts();
    setup_tss();
    setup_gdt();
    setup_idt();
    setup_paging();
    enable_interrupts();
    while (1) __asm__ volatile ("hlt");
}
