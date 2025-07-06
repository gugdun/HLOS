#ifndef _KERNEL_H
#define _KERNEL_H

#include <stdint.h>

void kernel_main(uint64_t fb_base, uint64_t fb_size, uint32_t fb_width, uint32_t fb_height);

#endif
