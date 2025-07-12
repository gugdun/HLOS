#ifndef _BOOT_GOP_H
#define _BOOT_GOP_H

#include <efi.h>
#include <kernel/graphics/framebuffer.h>

EFI_STATUS locate_gop(EFI_SYSTEM_TABLE *SystemTable, struct FramebufferParams *params);

#endif
