#ifndef _MEM_ENTRY_H
#define _MEM_ENTRY_H

#include <stdint.h>

struct MemoryMapEntry {
    uint32_t type;
    uint32_t pad;
    uint64_t physical_start;
    uint64_t virtual_start;
    uint64_t size_pages;
    uint64_t attribute;
};

typedef enum {
    ReservedMemory,
    KernelCode,
    KernelData,
    BootServicesCode,
    BootServicesData,
    RuntimeServicesCode,
    RuntimeServicesData,
    ConventionalMemory,
    UnusableMemory,
    ACPIReclaimMemory,
    ACPIMemoryNVS,
    MemoryMappedIO,
    MemoryMappedIOPortSpace,
    PalCode,
    PersistentMemory,
    UnacceptedMemory,
    MaxMemory
} MemoryMapEntryType;

#endif
