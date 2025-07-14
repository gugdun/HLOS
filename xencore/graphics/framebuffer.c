#include <string.h>
#include <math.h>

#include <xencore/graphics/framebuffer.h>
#include <xencore/graphics/fonts/8x8.h>
#include <xencore/io/serial.h>
#include <xencore/io/tty.h>

fb_color_t *fb_base   = (fb_color_t *)NULL;
fb_color_t *fb_buffer = (fb_color_t *)NULL;
uint8_t    *fb_font   = (uint8_t *)NULL;
size_t      fb_size   = 0;
uint32_t    fb_width  = 0;
uint32_t    fb_height = 0;
uint32_t    fb_ppsl   = 0; // Pixels Per Scan Line

FramebufferPixelFormat fb_format = 0;
struct FramebufferPixelBitmask fb_bitmask = { 0, 0, 0, 0 };
struct FramebufferBitmaskOffset bitmask_offset = { 0, 0, 0, 0 };

static inline void swap_u32(uint32_t *a, uint32_t *b) {
    uint32_t t = *a; *a = *b; *b = t;
}

void fb_init(struct FramebufferParams *params) {
    fb_base    = (uint32_t *)params->base;
    fb_font    = (uint8_t *)console_font_8x8;
    fb_size    = params->size;
    fb_width   = params->width;
    fb_height  = params->height;
    fb_ppsl    = params->ppsl;
    fb_format  = params->format;
    fb_bitmask = params->bitmask;

    if (fb_format == BitMaskFormat) {
        fb_bitmask.a = ~(fb_bitmask.r + fb_bitmask.g + fb_bitmask.b);
        while ((fb_bitmask.r >> bitmask_offset.r) == 0 && bitmask_offset.r < 32) bitmask_offset.r++;
        while ((fb_bitmask.g >> bitmask_offset.g) == 0 && bitmask_offset.g < 32) bitmask_offset.g++;
        while ((fb_bitmask.b >> bitmask_offset.b) == 0 && bitmask_offset.b < 32) bitmask_offset.b++;
        while ((fb_bitmask.a >> bitmask_offset.a) == 0 && bitmask_offset.a < 32) bitmask_offset.a++;
    }

    if (fb_base != NULL && fb_size > 0) {
        fb_clear(fb_color_rgb(CLEAR_COLOR_R, CLEAR_COLOR_G, CLEAR_COLOR_B));
    }

    tty_init(
        fb_color_rgb(TEXT_COLOR_R, TEXT_COLOR_G, TEXT_COLOR_B),
        fb_color_rgb(CLEAR_COLOR_R, CLEAR_COLOR_G, CLEAR_COLOR_B),
        console_font_8x8
    );

    if (fb_base == NULL || fb_size == 0) {
        tty_printf("[Framebuffer] Initialization failed...\n");
        return;
    }

    tty_printf(
        "[Framebuffer] Base: 0x%x, Size: 0x%x, Width: %u, Height: %u, Pixels Per Scanline: %u\n  Format: %d\n  R Mask: 0x%x\n  G Mask: 0x%x\n  B Mask: 0x%x\n  A Mask: 0x%x\n",
        fb_base, fb_size, fb_width, fb_height, fb_ppsl, (int)fb_format,
        fb_bitmask.r, fb_bitmask.g, fb_bitmask.b, fb_bitmask.a
    );
}

void fb_init_buffer(void *buffer)
{
    if (buffer == NULL) {
        tty_printf("[Framebuffer] Cannot enable double-buffering. Pointer is NULL.\n");
        return;
    }

    fb_buffer = (uint32_t *)buffer;
    fb_clear(fb_color_rgb(CLEAR_COLOR_R, CLEAR_COLOR_G, CLEAR_COLOR_B));

    tty_printf("[Framebuffer] Enabled double-buffering @ 0x%x\n", (uint64_t)fb_buffer);
}

void fb_present()
{
    if (fb_base == NULL) {
#ifdef HLOS_DEBUG
        tty_printf("[Framebuffer] Cannot present. Not initialized!\n");
#endif
        return;
    }

    if (fb_buffer == NULL) {
#ifdef HLOS_DEBUG
        tty_printf("[Framebuffer] Nothing to present. Single-buffered.\n");
#endif
        return;
    }

    memcpy((void *)fb_base, (const void *)fb_buffer, fb_size);
}

uint32_t fb_get_width()
{
    return fb_width;
}

uint32_t fb_get_height()
{
    return fb_height;
}

size_t fb_get_size()
{
    return fb_size;
}

bool fb_is_initialized()
{
    return (fb_base != NULL && fb_size > 0);
}

bool fb_is_double_buffered()
{
    return fb_buffer != NULL;
}

fb_color_t fb_color_rgb(float r, float g, float b)
{
    uint32_t r_channel;
    uint32_t g_channel;
    uint32_t b_channel;

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
            r_channel = ((uint32_t)(r * (float)(fb_bitmask.r >> bitmask_offset.r)) << bitmask_offset.r);
            g_channel = ((uint32_t)(g * (float)(fb_bitmask.g >> bitmask_offset.g)) << bitmask_offset.g);
            b_channel = ((uint32_t)(b * (float)(fb_bitmask.b >> bitmask_offset.b)) << bitmask_offset.b);
            return r_channel + g_channel + b_channel + fb_bitmask.a;

        default:
            return 0;
    }
}

uint32_t fb_color_rgba(float r, float g, float b, float a)
{
    uint32_t a_channel;
    
    if (a < 0.0f) a = 0.0f;
    else if (a > 1.0f) a = 1.0f;

    switch (fb_format) {
        case RGBA8Format:
        case BGRA8Format:
            return fb_color_rgb(r, g, b) + ((uint32_t)(a * 255.0f) << 24) - 0xFF000000;

        case BitMaskFormat:
            a_channel = ((uint32_t)(a * (float)(fb_bitmask.a >> bitmask_offset.a)) << bitmask_offset.a);
            return fb_color_rgb(r, g, b) - fb_bitmask.a + a_channel;

        default:
            return 0;
    }
}

void fb_clear(fb_color_t color)
{
    fb_color_t *ptr = fb_buffer;
    if (ptr == NULL) ptr = fb_base;
    const size_t pixels = fb_ppsl * fb_height;
    for (size_t y = 0; y < pixels; y += fb_ppsl) {
        for (size_t x = 0; x < fb_width; ++x) {
            ptr[x + y] = color;
        }
    }
}

void fb_set(fb_color_t color, uint32_t x, uint32_t y)
{
    if (x < fb_width && y < fb_height) {
        const size_t index = y * fb_ppsl + x;
        if (fb_buffer != NULL) fb_buffer[index] = color;
        else fb_base[index] = color;
    }
}

void fb_line(fb_color_t color, uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1)
{
    int dx = (int)fabs((float)x1 - (float)x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = -(int)fabs((float)y1 - (float)y0);
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    while (1) {
        fb_set(color, x0, y0);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void fb_hline(fb_color_t color, uint32_t x0, uint32_t x1, uint32_t y)
{
    if (y >= fb_width) return;
    if (x0 > x1) {
        swap_u32(&x0, &x1);
    }
    if (x1 >= fb_width) x1 = fb_width - 1;
    for (uint32_t x = x0; x <= x1; ++x) {
        fb_set(color, x, y);
    }
}

void fb_vline(fb_color_t color, uint32_t x, uint32_t y0, uint32_t y1)
{
    if (x >= fb_width) return;
    if (y0 > y1) {
        swap_u32(&y0, &y1);
    }
    if (y1 >= fb_height) y1 = fb_height - 1;
    for (uint32_t y = y0; y <= y1; ++y) {
        fb_set(color, x, y);
    }
}

void fb_rect(fb_color_t color, uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    if (w == 0 || h == 0) return;
    fb_hline(color, x, x + w - 1, y);         // top
    fb_hline(color, x, x + w - 1, y + h - 1); // bottom
    if (h > 2) {
        uint32_t left = x;
        uint32_t right = x + w - 1;
        if (left > right) swap_u32(&left, &right);
        for (uint32_t i = y + 1; i < y + h - 1; ++i) {
            fb_set(color, left, i);              // left
            fb_set(color, right, i);      // right
        }
    }
}

void fb_rect_fill(fb_color_t color, uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    if (w == 0 || h == 0) return;
    for (uint32_t i = 0; i < h; ++i) {
        fb_hline(color, x, x + w - 1, y + i);
    }
}

void fb_triangle(fb_color_t color, uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2) {
    fb_line(color, x0, y0, x1, y1);
    fb_line(color, x1, y1, x2, y2);
    fb_line(color, x2, y2, x0, y0);
}

void fb_triangle_fill(fb_color_t color, uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2) {
    // Sort vertices by y (y0 <= y1 <= y2)
    if (y0 > y1) { swap_u32(&y0, &y1); swap_u32(&x0, &x1); }
    if (y1 > y2) { swap_u32(&y1, &y2); swap_u32(&x1, &x2); }
    if (y0 > y1) { swap_u32(&y0, &y1); swap_u32(&x0, &x1); }

    // Handle degenerate triangle
    if (y0 == y2) return;

    // Helper to interpolate x between two points
    #define INTERP_X(xa, ya, xb, yb, y) \
        ((int)(xa) + ((int)(xb) - (int)(xa)) * ((int)(y) - (int)(ya)) / ((int)(yb) - (int)(ya)))

    for (uint32_t y = y0; y <= y2; ++y) {
        if (y < y1) {
            if (y1 != y0) {
                int xa = INTERP_X(x0, y0, x2, y2, y);
                int xb = INTERP_X(x0, y0, x1, y1, y);
                if (xa > xb) swap_u32((uint32_t*)&xa, (uint32_t*)&xb);
                fb_hline(color, xa, xb, y);
            }
        } else {
            if (y2 != y1) {
                int xa = INTERP_X(x0, y0, x2, y2, y);
                int xb = INTERP_X(x1, y1, x2, y2, y);
                if (xa > xb) swap_u32((uint32_t*)&xa, (uint32_t*)&xb);
                fb_hline(color, xa, xb, y);
            }
        }
    }
    
    #undef INTERP_X
}

void fb_draw_char(fb_color_t color, uint32_t x, uint32_t y, char c, const uint8_t *font) {
    const uint8_t *glyph = &font[(uint8_t)c * 8];
    for (uint32_t row = 0; row < 8; ++row) {
        uint8_t bits = glyph[row];
        for (uint32_t col = 0; col < 8; ++col) {
            if (bits & (1 << (7 - col))) {
                fb_set(color, x + col, y + row);
            }
        }
    }
}

void fb_scroll_up(uint32_t rows, fb_color_t color)
{
    if (rows == 0 || rows >= fb_height) return;
    fb_color_t *ptr = fb_buffer;
    if (ptr == NULL) ptr = fb_base;
    size_t line_size = fb_ppsl * sizeof(fb_color_t);
    size_t move_size = (fb_height - rows) * line_size;
    // Move lines up
    memmove(ptr, ptr + rows * fb_ppsl, move_size);
    // Clear new rows at the bottom
    for (uint32_t y = fb_height - rows; y < fb_height; ++y) {
        for (uint32_t x = 0; x < fb_width; ++x) {
            ptr[y * fb_ppsl + x] = color;
        }
    }
}
