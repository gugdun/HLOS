#include <efi.h>
#include <efilib.h>
#include <kernel/kernel.h>

EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    SystemTable->ConOut->OutputString(SystemTable->ConOut, L"[UEFI] Exiting UEFI Boot Services...\r\n");
    SystemTable->BootServices->ExitBootServices(ImageHandle, 0);

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

    kernel_main(fb_base, fb_size, fb_width, fb_height);

    return EFI_SUCCESS;
}
