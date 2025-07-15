#include <xencore/xenfs/test_sample.h>
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
        switch (hdr->typeflag) {
            case '0': type_name = "regular file"; break;
            case '1': type_name = "hard link"; break;
            case '2': type_name = "symbolic link"; break;
            case '3': type_name = "character device"; break;
            case '4': type_name = "block device"; break;
            case '5': type_name = "directory"; break;
            case '6': type_name = "FIFO"; break;
            case '7': type_name = "contiguous file"; break;
            default: break;
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

        // TODO: add file to VFS

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
