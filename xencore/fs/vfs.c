#include <xencore/fs/vfs.h>
#include <xencore/io/tty.h>

void vfs_init()
{
    tty_printf("[VFS] Initialized!\n");
}
