#ifndef _MMU_H
#define _MMU_H

#include <types.h>

typedef uint64 pd_t;

/*
 * Set identity paging, enable MMU
 */
void mmu_init(void);

#endif /* _MMU_H */