#ifndef _CPIO_H
#define _CPIO_H

#include <types.h>

extern void *initramfs_base;
extern void *initramfs_end;

void cpio_ls(char *cpio);
void cpio_cat(char *cpio, char *filename);

/*
 * Allocate a memory chunk and load the @filename program onto it. Then return
 * the address of the chunk by @output_data. @output_data needs to be passed 
 * to kfree() manually.
 *
 * Return output_data length, return 0 if no such file.
 */
uint32 cpio_load_prog(char *cpio, const char *filename, char **output_data);

void initramfs_init(void);

#endif /* _CPIO_H */