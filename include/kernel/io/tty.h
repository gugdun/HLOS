#ifndef _TTY_H
#define _TTY_H

#include <stdint.h>
#include <kernel/graphics/framebuffer.h>

#define TTY_COLS (fb_get_width() / 8)
#define TTY_ROWS (fb_get_height() / 8)

void tty_init(fb_color_t fg, fb_color_t bg, const uint8_t *font);
void tty_putc(char c);
void tty_puts(const char *s);
void tty_printf(const char* fmt, ...);
void tty_setcolor(fb_color_t fg, fb_color_t bg);
void tty_setpos(uint32_t x, uint32_t y);
uint32_t tty_getx(void);
uint32_t tty_gety(void);

#endif
