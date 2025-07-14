#include <anomalous/display.h>
#include <efilib.h>

EFI_STATUS setup_display(EFI_SYSTEM_TABLE *SystemTable, struct FramebufferParams *params)
{
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop = NULL;
    EFI_STATUS status = uefi_call_wrapper(
        SystemTable->BootServices->LocateProtocol, 3,
        &gEfiGraphicsOutputProtocolGuid, NULL, (void **)&gop
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
