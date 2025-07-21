#ifndef _TTY_H
#define _TTY_H

#include <stdint.h>

#include <xencore/graphics/framebuffer.h>

void tty_init(fb_color_t fg, fb_color_t bg, const uint8_t *font);
void tty_putc(char c);
void tty_puts(const char *s);
void tty_printf(const char* fmt, ...);
void tty_setcolor(fb_color_t fg, fb_color_t bg);
void tty_setpos(uint32_t x, uint32_t y);
void tty_setfont(const uint8_t *font);
void tty_reset();
uint32_t tty_getx(void);
uint32_t tty_gety(void);

#endif
