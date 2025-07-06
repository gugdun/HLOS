#include <kernel/graphics/framebuffer.h>
#include <kernel/io/print.h>
#include <string.h>

uint64_t fb_base, fb_size;
uint32_t fb_width, fb_height;

void fb_init(uint64_t base, uint64_t size, uint32_t width, uint32_t height)
{
    fb_base = base;
    fb_size = size;
    fb_width = width;
    fb_height = height;
    memset((void *)fb_base, (CLEAR_COLOR >> 8) & 0xFF, fb_size);
    kprintf("[Framebuffer] Base: 0x%x Size: 0x%x Width: %u Height: %u\n", base, size, width, height);
}
