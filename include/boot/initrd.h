#ifndef _BOOT_INITRD_H
#define _BOOT_INITRD_H

#include <efi.h>
#include <kernel/fs/initrd.h>

EFI_STATUS load_initrd(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable, struct InitrdParams *params);

#endif
