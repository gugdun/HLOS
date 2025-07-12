#ifndef _KERNEL_H
#define _KERNEL_H

#include <kernel/graphics/framebuffer.h>
#include <kernel/fs/initrd.h>
#include <kernel/memory/paging.h>

void kernel_main(struct FramebufferParams fb_params, struct InitrdParams initrd_params, struct MemoryMapParams memmap_params);

#endif
