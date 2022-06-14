#ifndef _FSINIT_H
#define _FSINIT_H

#include <fs/vfs.h>
#include <fs/tmpfs.h>
#include <fs/cpiofs.h>
#include <fs/uartfs.h>
#include <fs/framebufferfs.h>
#include <fs/fat32fs.h>

void fs_init(void);

#endif /* _FSINIT_H */