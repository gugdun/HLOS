#include <string.h>

#include <xencore/xenfs/test_sample.h>
#include <xencore/xenfs/vfs.h>
#include <xencore/xenio/tty.h>

static uint8_t *sample_base = NULL;
static size_t sample_size = 0;

static inline uint64_t oct2dec(const char *str, size_t size) {
    uint64_t result = 0;
    for (size_t i = 0; i < size && str[i]; ++i) {
        result = (result << 3) + (str[i] - '0'); // Multiply by 8
    }
    return result;
}

static void parse_tar(const void *tar_start, size_t tar_size) {
    const uint8_t *ptr = (const uint8_t *)tar_start;

    while ((size_t)ptr < (size_t)tar_start + tar_size - TAR_BLOCK_SIZE * 2) {
        struct TarHeader *hdr = (struct TarHeader *)ptr;

        // End of archive = two empty blocks
        if (hdr->name[0] == '\0') break;

        const uint64_t file_size = oct2dec(hdr->size, sizeof(hdr->size));
        const void *file_data = ptr + TAR_BLOCK_SIZE;

        char *type_name = "unknown";
        vfs_node_type_t vfs_type = VFS_NODE_DIR;

        switch (hdr->typeflag) {
            case TAR_REGULAR_FILE:
                type_name = "regular file";
                vfs_type = VFS_NODE_FILE;
                break;

            case TAR_HARD_LINK:
                type_name = "hard link";
                vfs_type = VFS_NODE_SYMLINK;
                break;

            case TAR_SYMBOLIC_LINK:
                type_name = "symbolic link";
                vfs_type = VFS_NODE_SYMLINK;
                break;
                
            case TAR_CHARACTER_DEVICE:
                type_name = "character device";
                vfs_type = VFS_NODE_FILE;
                break;

            case TAR_BLOCK_DEVICE:
                type_name = "block device";
                vfs_type = VFS_NODE_FILE;
                break;

            case TAR_DIRECTORY:
                type_name = "directory";
                break;

            case TAR_FIFO:
                type_name = "FIFO";
                vfs_type = VFS_NODE_FILE;
                break;

            case TAR_CONTIGUOUS_FILE:
                type_name = "contiguous file";
                vfs_type = VFS_NODE_FILE;
                break;

            default:
                break;
        }

        if (file_size > tar_size - (ptr - (const uint8_t *)tar_start) - TAR_BLOCK_SIZE) {
            tty_printf("[Test Sample] Error: File size exceeds remaining TAR size\n");
            break;
        }

        tty_printf(
            "[Test Sample] Found %s: %s (%u bytes)\n",
            type_name,
            hdr->name,
            file_size
        );

        // Add node to VFS
        size_t link_size = 0;
        char path[256] = "/";
        strcpy(&path[1], hdr->name);
        vfs_create(path, vfs_type);
        vfs_node_t *node = vfs_lookup(path);

        switch (node->type) {
            case VFS_NODE_FILE:
                node->file.size = file_size;
                node->file.data = (void *)vfs_alloc(file_size + 1);
                ((uint8_t *)node->file.data)[file_size] = 0;
                memcpy(node->file.data, file_data, file_size);
                break;
            
            case VFS_NODE_SYMLINK:
                link_size = strlen(hdr->linkname) + 1;
                node->symlink.target = (char *)vfs_alloc(link_size + 1);
                node->symlink.target[0] = '/';
                node->symlink.target[link_size] = 0;
                memcpy(&node->symlink.target[1], hdr->linkname, link_size - 1);
                break;
            
            default:
                break;
        }

        // Advance pointer: header + file data (rounded up to 512)
        size_t file_blocks = (file_size + TAR_BLOCK_SIZE - 1) / TAR_BLOCK_SIZE;
        ptr += TAR_BLOCK_SIZE + file_blocks * TAR_BLOCK_SIZE;
    }
}

void analyse_test_sample(struct TestSampleParams *params)
{
    sample_base = (uint8_t *)params->addr;
    sample_size = (size_t)params->size;
    tty_printf(
        "[Test Sample] %u bytes @ 0x%x\n",
        sample_size, (uint64_t)sample_base
    );
    parse_tar(sample_base, sample_size);
}
