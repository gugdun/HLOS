#include <kernel/cpu/fpu.h>
#include <kernel/cpu/gdt.h>
#include <kernel/interrupts/idt.h>
#include <kernel/interrupts/interrupts.h>
#include <kernel/io/serial.h>
#include <kernel/io/print.h>
#include <kernel/memory/paging.h>
#include <kernel/graphics/framebuffer.h>
#include <kernel/timer/pit.h>
#include <kernel/interrupts/pic.h>

void kernel_main(
    struct MemoryMapEntry *memory_map,
    uint64_t memory_map_size,
    uint64_t descriptor_size,
    uint64_t fb_base,
    uint64_t fb_size,
    uint32_t fb_width,
    uint32_t fb_height,
    FramebufferPixelFormat fb_format,
    struct FramebufferPixelBitmask fb_bitmask
) {
    serial_init();
    enable_fpu_sse();
    fb_init(fb_base, fb_size, fb_width, fb_height, fb_format, fb_bitmask);
    disable_interrupts();
    setup_tss();
    setup_gdt();
    setup_idt();
    setup_paging(memory_map, memory_map_size, descriptor_size, fb_base, fb_size);
    remap_pic();
    setup_pit(100);
    enable_interrupts();
    while (1) __asm__ volatile ("hlt");
}
