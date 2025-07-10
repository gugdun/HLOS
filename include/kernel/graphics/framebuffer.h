#ifndef _FRAMEBUFFER_H
#define _FRAMEBUFFER_H

#include <stdint.h>
#include <stddef.h>

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

void fb_init(uint64_t base, size_t size, uint32_t width, uint32_t height, FramebufferPixelFormat format, struct FramebufferPixelBitmask bitmask);
void fb_init_buffer(void *buffer);
void fb_present(void);
fb_color_t fb_color_rgb(float r, float g, float b);
fb_color_t fb_color_rgba(float r, float g, float b, float a);
void fb_clear(fb_color_t color);
void fb_set(fb_color_t color, uint32_t x, uint32_t y);

#endif
