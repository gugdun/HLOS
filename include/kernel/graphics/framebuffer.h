#ifndef _FRAMEBUFFER_H
#define _FRAMEBUFFER_H

#include <stdint.h>
#include <stddef.h>

#define CLEAR_COLOR 0x101010FF

typedef enum {
  RGBA8Format,
  BGRA8Format,
  BitMaskFormat
} FramebufferPixelFormat;

struct FramebufferPixelBitmask {
  uint32_t r;
  uint32_t g;
  uint32_t b;
};

void fb_init(uint64_t base, size_t size, uint32_t width, uint32_t height, FramebufferPixelFormat format, struct FramebufferPixelBitmask bitmask);
void fb_init_buffer(void *buffer);
void fb_present(void);

#endif
