#include <efi.h>
#include <efilib.h>

#include <boot/gop.h>
#include <boot/initrd.h>
#include <boot/memmap.h>
#include <kernel/kernel.h>

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
    InitializeLib(ImageHandle, SystemTable);
    
    UINTN map_key = 0;
    EFI_STATUS status = EFI_SUCCESS;

    struct FramebufferParams fb_params = { 0 };
    struct InitrdParams initrd_params = { 0 };
    struct MemoryMapParams memmap_params = { 0 };

    Print(L"[UEFI] Locating GOP...\r\n");
    status = locate_gop(SystemTable, &fb_params);
    if (EFI_ERROR(status)) {
        Print(L"[UEFI] Unable to locate GOP!\r\n");
        return status;
    }

    Print(L"[UEFI] Loading initrd...\r\n");
    status = load_initrd(ImageHandle, SystemTable, &initrd_params);
    if (EFI_ERROR(status)) {
        Print(L"[UEFI] Unable to load initrd!\r\n");
        return status;
    }

    Print(L"[UEFI] Getting memory map...\r\n");
    status = get_memory_map(SystemTable, &map_key, &memmap_params);
    if (EFI_ERROR(status)) {
        Print(L"[UEFI] Unable to get memory map!\r\n");
        return status;
    }

    Print(L"[UEFI] Exiting UEFI Boot Services...\r\n");
    uefi_call_wrapper(SystemTable->BootServices->ExitBootServices, 2, ImageHandle, map_key);

    kernel_main(fb_params, initrd_params, memmap_params);

    return EFI_SUCCESS;
}
