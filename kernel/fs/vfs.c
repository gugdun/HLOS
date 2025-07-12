#include <kernel/fs/vfs.h>
#include <kernel/io/tty.h>

void vfs_init()
{
    tty_printf("[VFS] Initialized!\n");
}
