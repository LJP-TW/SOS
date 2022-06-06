#ifndef _FSINIT_H
#define _FSINIT_H

#include <fs/vfs.h>
#include <fs/tmpfs.h>
#include <fs/cpiofs.h>

void fs_early_init(void);
void fs_init(void);

#endif /* _FSINIT_H */