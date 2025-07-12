#include <boot/gop.h>
#include <efilib.h>

static EFI_GUID GraphicsOutputProtocolGUID = {
    0x9042a9de, 0x23dc, 0x4a38,
    { 0x96, 0xfb, 0x7a, 0xde, 0xd0, 0x80, 0x51, 0x6a }
};

EFI_STATUS locate_gop(EFI_SYSTEM_TABLE *SystemTable, struct FramebufferParams *params)
{
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop = NULL;
    EFI_STATUS status = SystemTable->BootServices->LocateProtocol(
        &GraphicsOutputProtocolGUID,
        NULL,
        (void **)&gop
    );
    if (EFI_ERROR(status)) return status;

    params->base = (uint64_t)gop->Mode->FrameBufferBase;
    params->size = (size_t)gop->Mode->FrameBufferSize;
    params->width = (uint32_t)gop->Mode->Info->HorizontalResolution;
    params->height = (uint32_t)gop->Mode->Info->VerticalResolution;
    params->ppsl = (uint32_t)gop->Mode->Info->PixelsPerScanLine;
    params->format = (FramebufferPixelFormat)gop->Mode->Info->PixelFormat;
    params->bitmask.r = gop->Mode->Info->PixelInformation.RedMask;
    params->bitmask.g = gop->Mode->Info->PixelInformation.GreenMask;
    params->bitmask.b = gop->Mode->Info->PixelInformation.BlueMask;
    params->bitmask.a = gop->Mode->Info->PixelInformation.ReservedMask;

    return EFI_SUCCESS;
}
