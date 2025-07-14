#ifndef _KERNEL_H
#define _KERNEL_H

#ifdef ARCH_x86_64
#include <xencore/arch/x86_64/paging.h>
#endif

#include <xencore/graphics/framebuffer.h>
#include <xencore/fs/test_sample.h>

void resonance_cascade(struct FramebufferParams fb_params, struct TestSampleParams sample_params, struct MemoryMapParams memmap_params);

#endif
