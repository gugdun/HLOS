#include <boot/initrd.h>
#include <efilib.h>

static unsigned short initrd_path[] = L"\\EFI\\BOOT\\initrd.tar";

EFI_STATUS load_initrd(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable, struct InitrdParams *params) {
    EFI_LOADED_IMAGE_PROTOCOL *LoadedImage;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem;
    EFI_FILE_PROTOCOL *Root, *File;
    EFI_FILE_INFO *FileInfo;
    UINTN FileInfoSize = 0;
    VOID *InitrdBuffer = NULL;
    UINTN InitrdSize = 0;
    EFI_STATUS Status;

    // Get LoadedImage protocol
    Status = uefi_call_wrapper(
        SystemTable->BootServices->HandleProtocol, 3,
        ImageHandle,
        &gEfiLoadedImageProtocolGuid,
        (VOID **)&LoadedImage
    );
    if (EFI_ERROR(Status)) return Status;

    // Get SimpleFileSystem protocol
    Status = uefi_call_wrapper(
        SystemTable->BootServices->HandleProtocol, 3,
        LoadedImage->DeviceHandle,
        &gEfiSimpleFileSystemProtocolGuid,
        (VOID **)&FileSystem
    );
    if (EFI_ERROR(Status)) return Status;

    // Open volume (root dir)
    Status = uefi_call_wrapper(FileSystem->OpenVolume, 2, FileSystem, &Root);
    if (EFI_ERROR(Status)) return Status;

    // Open initrd file
    Status = uefi_call_wrapper(
        Root->Open, 5,
        Root,
        &File,
        initrd_path,
        EFI_FILE_MODE_READ,
        0
    );
    if (EFI_ERROR(Status)) return Status;

    // Get file size
    Status = uefi_call_wrapper(File->GetInfo, 4, File, &gEfiFileInfoGuid, &FileInfoSize, NULL);
    if (Status == EFI_BUFFER_TOO_SMALL) {
        Status = uefi_call_wrapper(
            SystemTable->BootServices->AllocatePool, 3,
            EfiLoaderData,
            FileInfoSize,
            (VOID **)&FileInfo
        );
        if (EFI_ERROR(Status)) return Status;

        Status = uefi_call_wrapper(File->GetInfo, 4, File, &gEfiFileInfoGuid, &FileInfoSize, FileInfo);
        if (EFI_ERROR(Status)) return Status;

        InitrdSize = FileInfo->FileSize;
        uefi_call_wrapper(SystemTable->BootServices->FreePool, 1, FileInfo);
    } else return Status;

    // Allocate memory
    Status = uefi_call_wrapper(
        SystemTable->BootServices->AllocatePool, 4,
        EfiLoaderData,
        InitrdSize,
        (VOID **)&InitrdBuffer
    );
    if (EFI_ERROR(Status)) return Status;

    // Read file into memory
    UINTN BytesRead = InitrdSize;
    Status = uefi_call_wrapper(File->Read, 3, File, &BytesRead, InitrdBuffer);
    if (EFI_ERROR(Status)) return Status;

    uefi_call_wrapper(File->Close, 1, File);
    params->addr = (uint64_t)InitrdBuffer;
    params->size = (uint64_t)BytesRead;

    return EFI_SUCCESS;
}
