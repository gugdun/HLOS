#include <efi.h>
#include <efilib.h>
#include <kernel/kernel.h>
#include <kernel/memory/mem_entry.h>
#include <kernel/graphics/framebuffer.h>

EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
    SystemTable->ConOut->OutputString(SystemTable->ConOut, L"[UEFI] Locating GOP...\r\n");
    static EFI_GUID GraphicsOutputProtocolGUID = {
        0x9042a9de, 0x23dc, 0x4a38,
        { 0x96, 0xfb, 0x7a, 0xde, 0xd0, 0x80, 0x51, 0x6a }
    };

    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
    EFI_STATUS status = SystemTable->BootServices->LocateProtocol(
        &GraphicsOutputProtocolGUID,
        NULL,
        (void **)&gop
    );

    if (EFI_ERROR(status)) {
        SystemTable->ConOut->OutputString(SystemTable->ConOut, L"[UEFI] Unable to locate GOP!\r\n");
    }

    EFI_PHYSICAL_ADDRESS fb_base = gop->Mode->FrameBufferBase;
    UINTN fb_size = gop->Mode->FrameBufferSize;
    UINT32 fb_width = gop->Mode->Info->HorizontalResolution;
    UINT32 fb_height = gop->Mode->Info->VerticalResolution;
    UINT32 fb_ppsl = gop->Mode->Info->PixelsPerScanLine;
    EFI_GRAPHICS_PIXEL_FORMAT fb_format = gop->Mode->Info->PixelFormat;
    EFI_PIXEL_BITMASK efi_pixel_bitmask = gop->Mode->Info->PixelInformation;

    SystemTable->ConOut->OutputString(SystemTable->ConOut, L"[UEFI] Getting memory map...\r\n");

    UINTN memory_map_size = 0;
    EFI_MEMORY_DESCRIPTOR *memory_map = NULL;
    UINTN map_key;
    UINTN descriptor_size;
    UINT32 descriptor_version;

    SystemTable->BootServices->GetMemoryMap(&memory_map_size, memory_map, &map_key, &descriptor_size, &descriptor_version);
    memory_map_size += 2 * descriptor_size;
    SystemTable->BootServices->AllocatePool(EfiLoaderData, memory_map_size, (VOID **)&memory_map);
    SystemTable->BootServices->GetMemoryMap(&memory_map_size, memory_map, &map_key, &descriptor_size, &descriptor_version);

    SystemTable->ConOut->OutputString(SystemTable->ConOut, L"[UEFI] Exiting UEFI Boot Services...\r\n");
    SystemTable->BootServices->ExitBootServices(ImageHandle, map_key);

    struct FramebufferPixelBitmask fb_bitmask = {
        efi_pixel_bitmask.RedMask,
        efi_pixel_bitmask.GreenMask,
        efi_pixel_bitmask.BlueMask,
        efi_pixel_bitmask.ReservedMask
    };

    kernel_main(
        (struct MemoryMapEntry *)memory_map,
        memory_map_size,
        descriptor_size,
        fb_base,
        fb_size,
        fb_width,
        fb_height,
        fb_ppsl,
        (FramebufferPixelFormat)fb_format,
        fb_bitmask
    );

    return EFI_SUCCESS;
}
