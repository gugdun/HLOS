#include <stdbool.h>

#include <kernel/io/serial.h>
#include <kernel/io/tty.h>
#include <kernel/cpu/fpu.h>
#include <kernel/cpu/gdt.h>
#include <kernel/interrupts/idt.h>
#include <kernel/interrupts/pic.h>
#include <kernel/interrupts/interrupts.h>
#include <kernel/memory/bitmap.h>
#include <kernel/memory/paging.h>
#include <kernel/timer/pit.h>
#include <kernel/timer/sleep.h>
#include <kernel/fs/vfs.h>
#include <kernel/fs/initrd.h>
#include <kernel/graphics/framebuffer.h>

#include <demo/triangle.h>

void kernel_main(struct FramebufferParams fb_params, struct InitrdParams initrd_params, struct MemoryMapParams memmap_params) {
    serial_init();
    fb_init(&fb_params);
    enable_fpu_sse();
    disable_interrupts();
    setup_tss();
    setup_gdt();
    setup_idt();
    setup_paging(&memmap_params, fb_params.base, fb_params.size);
    bitmap_init();
    vfs_init();
    parse_initrd(&initrd_params);
    remap_pic();
    setup_pit(100);
    enable_interrupts();

    struct DemoTriangleState state = demo_triangle_init();
    while (1) demo_triangle_tick(&state);
}
