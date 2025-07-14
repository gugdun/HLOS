#ifndef _AM_DISPLAY_H
#define _AM_DISPLAY_H

#include <efi.h>

#include <xencore/graphics/framebuffer.h>

EFI_STATUS setup_display(EFI_SYSTEM_TABLE *SystemTable, struct FramebufferParams *params);

#endif
