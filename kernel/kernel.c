#include <stdbool.h>

#include <kernel/io/serial.h>
#include <kernel/io/print.h>
#include <kernel/cpu/fpu.h>
#include <kernel/cpu/gdt.h>
#include <kernel/interrupts/idt.h>
#include <kernel/interrupts/pic.h>
#include <kernel/interrupts/interrupts.h>
#include <kernel/memory/bitmap.h>
#include <kernel/memory/paging.h>
#include <kernel/timer/pit.h>
#include <kernel/graphics/framebuffer.h>

void kernel_main(
    struct MemoryMapEntry *memory_map,
    size_t memory_map_size,
    size_t descriptor_size,
    uint64_t fb_base,
    size_t fb_size,
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
    bitmap_init();

    if (fb_size > 0) {
        bool fail = false;
        void *fb_buffer = alloc_page();
        if (fb_buffer == NULL) fail = true;
        
        size_t fb_pages = fb_size / PAGE_SIZE_2MB;
        for (size_t i = 0; i < fb_pages; ++i) {
            void *page = alloc_page();
            if (page == NULL) {
                fail = true;
                break;
            }
        }

        if (!fail) {
            fb_init_buffer(fb_buffer);
            fb_present();
        } else {
            kprintf("[Kernel] Failed to allocate memory for double-buffering.\n");
        }
    }

    remap_pic();
    setup_pit(100);
    enable_interrupts();

    while (1) __asm__ volatile ("hlt");
}
