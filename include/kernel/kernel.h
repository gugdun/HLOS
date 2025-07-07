#ifndef _KERNEL_H
#define _KERNEL_H

#include <kernel/memory/mem_entry.h>
#include <kernel/graphics/framebuffer.h>

void kernel_main(
    struct MemoryMapEntry *memory_map,
    uint64_t memory_map_size,
    uint64_t descriptor_size,
    uint64_t fb_base,
    uint64_t fb_size,
    uint32_t fb_width,
    uint32_t fb_height,
    FramebufferPixelFormat fb_format,
    struct FramebufferPixelBitmask fb_bitmask
);

#endif
