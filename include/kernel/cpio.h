#ifndef _CPIO_H
#define _CPIO_H

#include <types.h>

extern void *initramfs_base;
extern void *initramfs_end;

void cpio_ls(char *cpio);
void cpio_cat(char *cpio, char *filename);

// Return the address that the @filename program loaded to
char *cpio_load_prog(char *cpio, char *filename);

void initramfs_init(void);

#endif /* _CPIO_H */