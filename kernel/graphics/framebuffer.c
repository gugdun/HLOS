#include <kernel/graphics/framebuffer.h>
#include <kernel/io/print.h>
#include <string.h>

fb_color_t *fb_base   = (fb_color_t *)NULL;
fb_color_t *fb_buffer = (fb_color_t *)NULL;
size_t      fb_size   = 0;
uint32_t    fb_width  = 0;
uint32_t    fb_height = 0;

FramebufferPixelFormat fb_format = 0;
struct FramebufferPixelBitmask fb_bitmask = { 0, 0, 0, 0 };
struct FramebufferBitmaskOffset bitmask_offset = { 0, 0, 0, 0 };

void fb_init(uint64_t base, size_t size, uint32_t width, uint32_t height, FramebufferPixelFormat format, struct FramebufferPixelBitmask bitmask)
{
    fb_base = (uint32_t *)base;
    fb_size = size;
    fb_width = width;
    fb_height = height;
    fb_format = format;
    fb_bitmask = bitmask;

    if (fb_format == BitMaskFormat) {
        fb_bitmask.a = ~(fb_bitmask.r + fb_bitmask.g + fb_bitmask.b);
        while ((fb_bitmask.r >> bitmask_offset.r) == 0 && bitmask_offset.r < 32) bitmask_offset.r++;
        while ((fb_bitmask.g >> bitmask_offset.g) == 0 && bitmask_offset.g < 32) bitmask_offset.g++;
        while ((fb_bitmask.b >> bitmask_offset.b) == 0 && bitmask_offset.b < 32) bitmask_offset.b++;
        while ((fb_bitmask.a >> bitmask_offset.a) == 0 && bitmask_offset.a < 32) bitmask_offset.a++;
    }

    if (fb_base == NULL || fb_size == 0) {
        kprintf("[Framebuffer] Initialization failed...\n");
        return;
    }

    fb_clear(fb_color_rgb(CLEAR_COLOR_R, CLEAR_COLOR_G, CLEAR_COLOR_B));
    kprintf(
        "[Framebuffer] Base: 0x%x Size: 0x%x Width: %u Height: %u\n  Format: %d\n  R Mask: 0x%x\n  G Mask: 0x%x\n  B Mask: 0x%x\n",
        base, size, width, height,
        format, bitmask.r, bitmask.g, bitmask.b
    );
}

void fb_init_buffer(void *buffer)
{
    if (buffer == NULL) {
        kprintf("[Framebuffer] Cannot enable double-buffering. Pointer is NULL.\n");
        return;
    }

    fb_buffer = (uint32_t *)buffer;
    fb_clear(fb_color_rgb(CLEAR_COLOR_R, CLEAR_COLOR_G, CLEAR_COLOR_B));

    kprintf("[Framebuffer] Enabled double-buffering @ 0x%x\n", (uint64_t)fb_buffer);
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

uint32_t fb_color_rgb(float r, float g, float b)
{
    if (r < 0.0f) r = 0.0f;
    else if (r > 1.0f) r = 1.0f;
    if (g < 0.0f) g = 0.0f;
    else if (g > 1.0f) g = 1.0f;
    if (b < 0.0f) b = 0.0f;
    else if (b > 1.0f) b = 1.0f;

    switch (fb_format) {
        case RGBA8Format:
            return ((uint32_t)(b * 255.0f) << 16) + ((uint32_t)(g * 255.0f) << 8) + (uint32_t)(r * 255.0f) + 0xFF000000;

        case BGRA8Format:
            return ((uint32_t)(r * 255.0f) << 16) + ((uint32_t)(g * 255.0f) << 8) + (uint32_t)(b * 255.0f) + 0xFF000000;

        case BitMaskFormat:
            const uint32_t r_channel = ((uint32_t)(r * (float)(fb_bitmask.r >> bitmask_offset.r)) << bitmask_offset.r);
            const uint32_t g_channel = ((uint32_t)(g * (float)(fb_bitmask.g >> bitmask_offset.g)) << bitmask_offset.g);
            const uint32_t b_channel = ((uint32_t)(b * (float)(fb_bitmask.b >> bitmask_offset.b)) << bitmask_offset.b);
            return r_channel + g_channel + b_channel + fb_bitmask.a;

        default:
            return 0;
    }
}

uint32_t fb_color_rgba(float r, float g, float b, float a)
{
    if (a < 0.0f) a = 0.0f;
    else if (a > 1.0f) a = 1.0f;

    switch (fb_format) {
        case RGBA8Format:
        case BGRA8Format:
            return fb_color_rgb(r, g, b) + ((uint32_t)(a * 255.0f) << 24) - 0xFF000000;

        case BitMaskFormat:
            const uint32_t a_channel = ((uint32_t)(a * (float)(fb_bitmask.a >> bitmask_offset.a)) << bitmask_offset.a);
            return fb_color_rgb(r, g, b) - fb_bitmask.a + a_channel;

        default:
            return 0;
    }
}

void fb_clear(fb_color_t color)
{
    fb_color_t *ptr = fb_buffer;
    if (ptr == NULL) ptr = fb_base;
    const size_t pixels = fb_width * fb_height;
    for (size_t y = 0; y < pixels; y += fb_width) {
        for (size_t x = 0; x < fb_width; ++x) {
            ptr[x + y] = color;
        }
    }
}

void fb_set(fb_color_t color, uint32_t x, uint32_t y)
{
    if (x < fb_width && y < fb_height) {
        const size_t index = y * fb_width + x;
        if (fb_buffer != NULL) fb_buffer[index] = color;
        else fb_base[index] = color;
    }
}
