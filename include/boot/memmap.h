#ifndef _BOOT_MEMMAP_H
#define _BOOT_MEMMAP_H

#include <efi.h>
#include <kernel/memory/paging.h>

EFI_STATUS get_memory_map(EFI_SYSTEM_TABLE *SystemTable, UINTN *map_key, struct MemoryMapParams *params);

#endif
