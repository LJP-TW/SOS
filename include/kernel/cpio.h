#ifndef _CPIO_H
#define _CPIO_H

#include <types.h>

extern void *initramfs_base;
extern void *initramfs_end;

void initramfs_init(void);

#endif /* _CPIO_H */