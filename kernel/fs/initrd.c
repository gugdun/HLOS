#include <kernel/fs/initrd.h>
#include <kernel/io/tty.h>

static uint8_t *initrd_base = NULL;
static size_t initrd_size = 0;

void parse_initrd(struct InitrdParams *params)
{
    initrd_base = (uint8_t *)params->addr;
    initrd_size = (size_t)params->size;
    tty_printf(
        "[Initrd] %u bytes @ 0x%x\n",
        initrd_size, (uint64_t)initrd_base
    );
    tty_printf(
        "         | 0x00 0x01 0x02 0x03\n  -------+--------------------\n  0x0000 | 0x%x 0x%x 0x%x 0x%x\n",
        initrd_base[0], initrd_base[1], initrd_base[2], initrd_base[3]
    );
}
