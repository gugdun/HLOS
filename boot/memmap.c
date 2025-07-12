#include <boot/memmap.h>
#include <efilib.h>

EFI_STATUS get_memory_map(EFI_SYSTEM_TABLE *SystemTable, UINTN *map_key, struct MemoryMapParams *params)
{
    UINTN memory_map_size = 0;
    EFI_MEMORY_DESCRIPTOR *memory_map = NULL;
    UINTN descriptor_size;
    UINT32 descriptor_version;
    EFI_STATUS Status;

    Status = SystemTable->BootServices->GetMemoryMap(
        &memory_map_size,
        memory_map,
        map_key,
        &descriptor_size,
        &descriptor_version
    );
    if (Status == EFI_BUFFER_TOO_SMALL) {
        memory_map_size += 2 * descriptor_size;
    
        Status = SystemTable->BootServices->AllocatePool(
            EfiLoaderData,
            memory_map_size,
            (VOID **)&memory_map
        );
        if (EFI_ERROR(Status)) return Status;
        
        Status = SystemTable->BootServices->GetMemoryMap(
            &memory_map_size,
            memory_map,
            map_key,
            &descriptor_size,
            &descriptor_version
        );
        if (EFI_ERROR(Status)) return Status;
    } else return Status;

    params->memory_map = (struct MemoryMapEntry *)memory_map;
    params->memory_map_size = (size_t)memory_map_size;
    params->descriptor_size = (size_t)descriptor_size;

    return EFI_SUCCESS;
}
