#include <efi.h>
#include <efilib.h>

#include <anomalous/display.h>
#include <anomalous/sample.h>
#include <anomalous/topology.h>
#include <xencore/core.h>

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
    InitializeLib(ImageHandle, SystemTable);
    
    UINTN map_key = 0;
    EFI_STATUS status = EFI_SUCCESS;

    struct FramebufferParams fb_params = { 0 };
    struct TestSampleParams sample_params = { 0 };
    struct MemoryMapParams memmap_params = { 0 };

    Print(L"[Anomalous Materials] Preparing test chamber display buffer...\r\n");
    status = setup_display(SystemTable, &fb_params);
    if (EFI_ERROR(status)) {
        Print(L"[Anomalous Materials] Unable to setup display!\r\n");
        return status;
    }

    Print(L"[Anomalous Materials] Retrieving test_sample.tar...\r\n");
    status = load_sample(ImageHandle, SystemTable, &sample_params);
    if (EFI_ERROR(status)) {
        Print(L"[Anomalous Materials] Unable to retrieve test sample!\r\n");
        return status;
    }

    Print(L"[Anomalous Materials] Acquiring system topology...\r\n");
    status = acquire_topology(SystemTable, &map_key, &memmap_params);
    if (EFI_ERROR(status)) {
        Print(L"[Anomalous Materials] Unable to acquire system topology!\r\n");
        return status;
    }

    Print(L"[Anomalous Materials] Injecting sample into analysis chamber...\r\n");
    uefi_call_wrapper(SystemTable->BootServices->ExitBootServices, 2, ImageHandle, map_key);

    // Scientist A: "A resonance cascade scenario is extremely unlikely."
    // Scientist B: "Gordon doesn't need to hear all this. He's a highly trained professional."
    // Scientist A: "We've assured the administrator that nothing will go wrong."
    // Scientist B: "Ah yes, you're right. Gordon, we have complete confidence in you."
    // Scientist A: "Well, go ahead. Let's not delay this any further."
    resonance_cascade(fb_params, sample_params, memmap_params);

    return EFI_SUCCESS;
}
