#ifndef _UARTFS_H
#define _UARTFS_H

#include <fs/vfs.h>

struct filesystem *uartfs_init(void);

#endif /* _UARTFS_H */