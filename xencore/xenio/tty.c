#include <stdarg.h>

#include <xencore/graphics/framebuffer.h>
#include <xencore/graphics/fonts/8x14.h>
#include <xencore/xenio/serial.h>
#include <xencore/xenio/tty.h>

static uint32_t tty_x = 0, tty_y = 0;
static uint32_t tty_cols = 0, tty_rows = 0;
static fb_color_t tty_fg = 0xFFFFFFFF, tty_bg = 0xFF000000;
static const uint8_t *tty_font = NULL;

void tty_init(fb_color_t fg, fb_color_t bg, const uint8_t *font) {
    tty_x = 0;
    tty_y = 0;
    tty_fg = fg;
    tty_bg = bg;
    tty_font = font ? font : (const uint8_t*)console_font_8x14;
    tty_cols = fb_get_width() / font[0];
    tty_rows = fb_get_height() / font[1];
    tty_printf("[TTY] Initialized with foreground color: 0x%x, background color: 0x%x\n", tty_fg, tty_bg);
}

void tty_setcolor(fb_color_t fg, fb_color_t bg) {
    tty_fg = fg;
    tty_bg = bg;
}

void tty_setpos(uint32_t x, uint32_t y) {
    tty_x = x;
    tty_y = y;
}

uint32_t tty_getx(void) { return tty_x; }
uint32_t tty_gety(void) { return tty_y; }

void tty_reset()
{
    tty_x = 0;
    tty_y = 0;
    fb_clear(tty_bg);
}

void tty_setfont(const uint8_t *font)
{
    if (font != NULL) {
        tty_font = font;
        tty_cols = fb_get_width() / font[0];
        tty_rows = fb_get_height() / font[1];
        tty_reset();
    }
}

static void tty_scroll(void) {
    const uint8_t font_height = tty_font[1];
    fb_scroll_up(font_height, tty_bg);
    if (tty_y > 0) tty_y--;
}

void tty_putc(char c) {
    serial_print_char(c);
    if (!fb_is_initialized()) return;
    
    if (!tty_font) tty_font = (const uint8_t*)console_font_8x14;
    const uint8_t font_w = tty_font[0];
    const uint8_t font_h = tty_font[1];
    switch (c) {
        case '\n':
            tty_x = 0;
            tty_y++;
            if (tty_y >= tty_rows) { tty_scroll(); tty_y = tty_rows - 1; }
            break;
        case '\r':
            tty_x = 0;
            break;
        case '\t':
            tty_x = (tty_x + 4) & ~(4 - 1);
            if (tty_x >= tty_cols) { tty_x = 0; tty_y++; }
            if (tty_y >= tty_rows) { tty_scroll(); tty_y = tty_rows - 1; }
            break;
        case '\b':
            if (tty_x > 0) tty_x--;
            else if (tty_y > 0) { tty_y--; tty_x = tty_cols - 1; }
            fb_draw_char(tty_bg, tty_x * font_w, tty_y * font_h, ' ', tty_font);
            break;
        default:
            fb_draw_char(tty_fg, tty_x * font_w, tty_y * font_h, c, tty_font);
            tty_x++;
            if (tty_x >= tty_cols) { tty_x = 0; tty_y++; }
            if (tty_y >= tty_rows) { tty_scroll(); tty_y = tty_rows - 1; }
            break;
    }
}

void tty_puts(const char *s) {
    while (*s) tty_putc(*s++);
}

void tty_printf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    while (*fmt) {
        if (*fmt != '%') {
            tty_putc(*fmt++);
            continue;
        }
        ++fmt;
        switch (*fmt) {
            case 'd':
            case 'i': {
                long long val = va_arg(args, long long);
                char buf[32];
                char *p = buf + sizeof(buf);
                int neg = val < 0;
                if (neg) val = -val;
                *--p = 0;
                if (val == 0) *--p = '0';
                while (val) { *--p = '0' + (val % 10); val /= 10; }
                if (neg) *--p = '-';
                tty_puts(p);
                break;
            }
            case 'u': {
                unsigned long long val = va_arg(args, unsigned long long);
                char buf[32];
                char *p = buf + sizeof(buf);
                *--p = 0;
                if (val == 0) *--p = '0';
                while (val) { *--p = '0' + (val % 10); val /= 10; }
                tty_puts(p);
                break;
            }
            case 'x': {
                unsigned long long val = va_arg(args, unsigned long long);
                char buf[32];
                char *p = buf + sizeof(buf);
                const char* digits = "0123456789ABCDEF";
                *--p = 0;
                if (val == 0) *--p = '0';
                while (val) { *--p = digits[val % 16]; val /= 16; }
                tty_puts(p);
                break;
            }
            case 'f': {
                double value = va_arg(args, double);
                if (value < 0) { tty_putc('-'); value = -value; }
                long long int_part = (long long)value;
                double frac_part = value - int_part;
                char buf[32];
                char *p = buf + sizeof(buf);
                *--p = 0;
                if (int_part == 0) *--p = '0';
                long long v = int_part;
                while (v) { *--p = '0' + (v % 10); v /= 10; }
                tty_puts(p);
                tty_putc('.');
                for (int i = 0; i < 6; ++i) frac_part *= 10;
                unsigned long long frac = (unsigned long long)frac_part;
                char fbuf[8];
                char *fp = fbuf + sizeof(fbuf);
                *--fp = 0;
                for (int i = 0; i < 6; ++i) { *--fp = '0' + (frac % 10); frac /= 10; }
                tty_puts(fp);
                break;
            }
            case 's': {
                const char* str = va_arg(args, const char*);
                tty_puts(str ? str : "(null)");
                break;
            }
            case 'c':
                tty_putc((char)va_arg(args, int));
                break;
            case '%':
                tty_putc('%');
                break;
            default:
                tty_putc('%');
                tty_putc(*fmt);
                break;
        }
        ++fmt;
    }
    va_end(args);
}


