#ifndef _AM_TOPOLOGY_H
#define _AM_TOPOLOGY_H

#ifdef ARCH_x86_64
#include <xencore/arch/x86_64/paging.h>
#endif

#include <efi.h>

EFI_STATUS acquire_topology(EFI_SYSTEM_TABLE *SystemTable, UINTN *map_key, struct MemoryMapParams *params);

#endif
