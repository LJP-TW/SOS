#include <fs/fsinit.h>
#include <mm/mm.h>
#include <panic.h>

void fs_early_init(void)
{
    struct filesystem *tmpfs, *cpiofs;

    vfs_init();

    tmpfs = tmpfs_init();
    cpiofs = cpiofs_init();
    register_filesystem(tmpfs);
    register_filesystem(cpiofs);

    vfs_init_rootmount(tmpfs);
}

void fs_init(void)
{
    vfs_mkdir("/initramfs");
    vfs_mount("/initramfs", "cpiofs");
}