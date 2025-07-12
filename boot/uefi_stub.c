#include <efi.h>
#include <efilib.h>

#include <boot/gop.h>
#include <boot/initrd.h>
#include <boot/memmap.h>
#include <kernel/kernel.h>

#define Print(EFI_ST, Message) EFI_ST->ConOut->OutputString(EFI_ST->ConOut, Message) 

EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
    UINTN map_key = 0;
    EFI_STATUS status = EFI_SUCCESS;

    struct FramebufferParams fb_params = { 0 };
    struct InitrdParams initrd_params = { 0 };
    struct MemoryMapParams memmap_params = { 0 };

    Print(SystemTable, L"[UEFI] Locating GOP...\r\n");
    status = locate_gop(SystemTable, &fb_params);
    if (EFI_ERROR(status)) {
        Print(SystemTable, L"[UEFI] Unable to locate GOP!\r\n");
        return status;
    }

    Print(SystemTable, L"[UEFI] Loading initrd...\r\n");
    status = load_initrd(ImageHandle, SystemTable, &initrd_params);
    if (EFI_ERROR(status)) {
        Print(SystemTable, L"[UEFI] Unable to load initrd!\r\n");
        return status;
    }

    Print(SystemTable, L"[UEFI] Getting memory map...\r\n");
    status = get_memory_map(SystemTable, &map_key, &memmap_params);
    if (EFI_ERROR(status)) {
        Print(SystemTable, L"[UEFI] Unable to get memory map!\r\n");
        return status;
    }

    Print(SystemTable, L"[UEFI] Exiting UEFI Boot Services...\r\n");
    SystemTable->BootServices->ExitBootServices(ImageHandle, map_key);

    kernel_main(fb_params, initrd_params, memmap_params);

    return EFI_SUCCESS;
}
