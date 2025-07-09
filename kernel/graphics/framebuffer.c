#include <kernel/graphics/framebuffer.h>
#include <kernel/io/print.h>
#include <string.h>

uint8_t *fb_base   = (uint8_t *)NULL;
uint8_t *fb_buffer = (uint8_t *)NULL;
size_t   fb_size   = 0;
uint32_t fb_width  = 0;
uint32_t fb_height = 0;

FramebufferPixelFormat fb_format;
struct FramebufferPixelBitmask fb_bitmask;

void fb_init(uint64_t base, size_t size, uint32_t width, uint32_t height, FramebufferPixelFormat format, struct FramebufferPixelBitmask bitmask)
{
    fb_base = (uint8_t *)base;
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

void fb_init_buffer(void *buffer)
{
    fb_buffer = (uint8_t *)buffer;
    memset((void *)fb_buffer, (CLEAR_COLOR >> 8) & 0xFF, fb_size);
#ifdef HLOS_DEBUG
    kprintf("[Framebuffer] Enabled double-buffering @ 0x%x\n", (uint64_t)fb_buffer);
#endif
}

void fb_present()
{
    if (fb_base == NULL) {
#ifdef HLOS_DEBUG
        kprintf("[Framebuffer] Cannot present. Not initialized!\n");
#endif
        return;
    }

    if (fb_buffer == NULL) {
#ifdef HLOS_DEBUG
        kprintf("[Framebuffer] Nothing to present. Single-buffered.\n");
#endif
        return;
    }

    memcpy((void *)fb_base, (const void *)fb_buffer, fb_size);
}
