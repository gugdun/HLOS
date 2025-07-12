#ifndef _FRAMEBUFFER_H
#define _FRAMEBUFFER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define TEXT_COLOR_R 0.9f
#define TEXT_COLOR_G 0.9f
#define TEXT_COLOR_B 0.9f

#define CLEAR_COLOR_R 0.1f
#define CLEAR_COLOR_G 0.1f
#define CLEAR_COLOR_B 0.1f

typedef uint32_t fb_color_t;

typedef enum {
  RGBA8Format,
  BGRA8Format,
  BitMaskFormat
} FramebufferPixelFormat;

struct FramebufferPixelBitmask {
  uint32_t r;
  uint32_t g;
  uint32_t b;
  uint32_t a;
};

struct FramebufferBitmaskOffset {
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t a;
};

struct FramebufferParams {
  uint64_t base;
  size_t size;
  uint32_t width;
  uint32_t height;
  uint32_t ppsl;
  FramebufferPixelFormat format;
  struct FramebufferPixelBitmask bitmask;
};

void fb_init(struct FramebufferParams *params);

void fb_init_buffer(void *buffer);
void fb_present(void);
uint32_t fb_get_width(void);
uint32_t fb_get_height(void);
size_t fb_get_size(void);
bool fb_is_initialized();
fb_color_t fb_color_rgb(float r, float g, float b);
fb_color_t fb_color_rgba(float r, float g, float b, float a);
void fb_clear(fb_color_t color);
void fb_set(fb_color_t color, uint32_t x, uint32_t y);
void fb_line(fb_color_t color, uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1);
void fb_hline(fb_color_t color, uint32_t x0, uint32_t x1, uint32_t y);
void fb_vline(fb_color_t color, uint32_t x, uint32_t y0, uint32_t y1);
void fb_rect(fb_color_t color, uint32_t x, uint32_t y, uint32_t w, uint32_t h);
void fb_rect_fill(fb_color_t color, uint32_t x, uint32_t y, uint32_t w, uint32_t h);
void fb_triangle(fb_color_t color, uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2);
void fb_triangle_fill(fb_color_t color, uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2);
void fb_draw_char(fb_color_t color, uint32_t x, uint32_t y, char c, const uint8_t *font);
void fb_scroll_up(uint32_t rows, fb_color_t color);

#endif
