#include <stdbool.h>
#include <lib/math.h>

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
#include <kernel/graphics/framebuffer.h>

void kernel_main(
    struct MemoryMapEntry *memory_map,
    size_t memory_map_size,
    size_t descriptor_size,
    uint64_t fb_base,
    size_t fb_size,
    uint32_t fb_width,
    uint32_t fb_height,
    uint32_t fb_ppsl,
    FramebufferPixelFormat fb_format,
    struct FramebufferPixelBitmask fb_bitmask
) {
    serial_init();
    fb_init(fb_base, fb_size, fb_width, fb_height, fb_ppsl, fb_format, fb_bitmask);
    enable_fpu_sse();
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
        } else {
            tty_printf("[Kernel] Failed to allocate memory for double-buffering.\n");
        }
    }

    remap_pic();
    setup_pit(1000);
    enable_interrupts();

    float angle = 0.0f;
    const fb_color_t clear_color = fb_color_rgb(0.1f, 0.1f, 0.1f);
    const fb_color_t triangle_color = fb_color_rgb(0.1f, 0.7f, 0.2f);

    vector2 v1 = {  0.0f,  0.5f };
    vector2 v2 = { -0.5f, -0.5f };
    vector2 v3 = {  0.5f, -0.5f };

    while (1) {
        fb_clear(clear_color);

        matrix2x2 rotation_matrix = {
            .m = {
                {cos(angle), -sin(angle)},
                {sin(angle), cos(angle)}
            }
        };

        vector2 rotated_v1 = matrix2x2_mul_vector2(rotation_matrix, v1);
        vector2 rotated_v2 = matrix2x2_mul_vector2(rotation_matrix, v2);
        vector2 rotated_v3 = matrix2x2_mul_vector2(rotation_matrix, v3);

        float aspect = (float)fb_width / (float)fb_height;
        rotated_v1.x /= aspect;
        rotated_v2.x /= aspect;
        rotated_v3.x /= aspect;

        rotated_v1.x = (rotated_v1.x + 1.0f) * (fb_width / 2.0f);
        rotated_v1.y = (rotated_v1.y + 1.0f) * (fb_height / 2.0f);
        rotated_v2.x = (rotated_v2.x + 1.0f) * (fb_width / 2.0f);
        rotated_v2.y = (rotated_v2.y + 1.0f) * (fb_height / 2.0f);
        rotated_v3.x = (rotated_v3.x + 1.0f) * (fb_width / 2.0f);
        rotated_v3.y = (rotated_v3.y + 1.0f) * (fb_height / 2.0f);

        fb_triangle_fill(
            triangle_color,
            rotated_v1.x, rotated_v1.y,
            rotated_v2.x, rotated_v2.y,
            rotated_v3.x, rotated_v3.y
        );

        fb_present();
        ksleep(16);

        angle += 0.016f;
        if (angle > 2.0f * M_PI) angle -= 2.0f * M_PI;
    }
}
