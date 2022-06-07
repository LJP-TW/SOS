#include <fs/fsinit.h>
#include <mm/mm.h>
#include <panic.h>

void fs_init(void)
{
    struct filesystem *tmpfs, *cpiofs, *uartfs, *fbfs;

    vfs_init();

    tmpfs = tmpfs_init();
    cpiofs = cpiofs_init();
    uartfs = uartfs_init();
    fbfs = framebufferfs_init();
    register_filesystem(tmpfs);
    register_filesystem(cpiofs);
    register_filesystem(uartfs);
    register_filesystem(fbfs);

    vfs_init_rootmount(tmpfs);

    vfs_mkdir("/initramfs");
    vfs_mount("/initramfs", "cpiofs");

    vfs_mkdir("/dev");

    vfs_mkdir("/dev/uart");
    vfs_mount("/dev/uart", "uartfs");

    vfs_mkdir("/dev/framebuffer");
    vfs_mount("/dev/framebuffer", "framebufferfs");
}