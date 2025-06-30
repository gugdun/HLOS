#include <efi.h>
#include <efilib.h>
#include <kernel/kernel.h>

EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    SystemTable->ConOut->OutputString(SystemTable->ConOut, L"Exiting UEFI Boot Services...\r\n");
    SystemTable->BootServices->ExitBootServices(ImageHandle, 0);

    kernel_main();

    return EFI_SUCCESS;
}
