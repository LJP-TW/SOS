#ifndef _MMU_H
#define _MMU_H

#include <types.h>

#define PAGE_TABLE_SIZE 0x1000

#define PT_R    0x0001
#define PT_W    0x0002
#define PT_X    0x0004

typedef uint64 pd_t;

/*
 * Set identity paging, enable MMU
 */
void mmu_init(void);

pd_t *pt_create(void);
void pt_free(pd_t *pt);

/*
 * Create a @size mapping of @va -> @pa.
 * @pt is PGD.
 */
void pt_map(pd_t *pt, void *va, uint64 size, void *pa, uint64 flag);

#endif /* _MMU_H */