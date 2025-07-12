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
    Status = SystemTable->BootServices->HandleProtocol(
        ImageHandle,
        &gEfiLoadedImageProtocolGuid,
        (VOID **)&LoadedImage
    );
    if (EFI_ERROR(Status)) return Status;

    // Get SimpleFileSystem protocol
    Status = SystemTable->BootServices->HandleProtocol(
        LoadedImage->DeviceHandle,
        &gEfiSimpleFileSystemProtocolGuid,
        (VOID **)&FileSystem
    );
    if (EFI_ERROR(Status)) return Status;

    // Open volume (root dir)
    Status = FileSystem->OpenVolume(FileSystem, &Root);
    if (EFI_ERROR(Status)) return Status;

    // Open initrd file
    Status = Root->Open(
        Root,
        &File,
        initrd_path,
        EFI_FILE_MODE_READ,
        0
    );
    if (EFI_ERROR(Status)) return Status;

    // Get file size
    Status = File->GetInfo(File, &gEfiFileInfoGuid, &FileInfoSize, NULL);
    if (Status == EFI_BUFFER_TOO_SMALL) {
        Status = SystemTable->BootServices->AllocatePool(
            EfiLoaderData,
            FileInfoSize,
            (VOID **)&FileInfo
        );
        if (EFI_ERROR(Status)) return Status;

        Status = File->GetInfo(File, &gEfiFileInfoGuid, &FileInfoSize, FileInfo);
        if (EFI_ERROR(Status)) return Status;

        InitrdSize = FileInfo->FileSize;
        SystemTable->BootServices->FreePool(FileInfo);
    } else return Status;

    // Allocate memory
    Status = SystemTable->BootServices->AllocatePool(EfiLoaderData, InitrdSize, (VOID **)&InitrdBuffer);
    if (EFI_ERROR(Status)) return Status;

    // Read file into memory
    UINTN BytesRead = InitrdSize;
    Status = File->Read(File, &BytesRead, InitrdBuffer);
    if (EFI_ERROR(Status)) return Status;

    File->Close(File);
    params->addr = (uint64_t)InitrdBuffer;
    params->size = (uint64_t)BytesRead;

    return EFI_SUCCESS;
}
