#include <kernel/graphics/framebuffer.h>
#include <kernel/io/print.h>
#include <string.h>

uint64_t fb_base;
uint64_t fb_size;
uint32_t fb_width;
uint32_t fb_height;
FramebufferPixelFormat fb_format;
struct FramebufferPixelBitmask fb_bitmask;

void fb_init(uint64_t base, uint64_t size, uint32_t width, uint32_t height, FramebufferPixelFormat format, struct FramebufferPixelBitmask bitmask)
{
    fb_base = base;
    fb_size = size;
    fb_width = width;
    fb_height = height;
    fb_format = format;
    fb_bitmask = bitmask;
    memset((void *)fb_base, (CLEAR_COLOR >> 8) & 0xFF, fb_size);
    kprintf(
        "[Framebuffer] Base: 0x%x Size: 0x%x Width: %u Height: %u\n  Format: %d\n  R Mask: 0x%x\n  G Mask: 0x%x\n  B Mask: 0x%x\n",
        base, size, width, height,
        format, bitmask.r, bitmask.g, bitmask.b
    );
}
