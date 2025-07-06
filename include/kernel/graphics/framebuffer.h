#ifndef _FRAMEBUFFER_H
#define _FRAMEBUFFER_H

#include <stdint.h>

#define CLEAR_COLOR 0x101010FF

void fb_init(uint64_t base, uint64_t size, uint32_t width, uint32_t height);

#endif
