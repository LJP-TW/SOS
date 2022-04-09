#ifndef _PAGE_ALLOC_H
#define _PAGE_ALLOC_H

#include <types.h>

#define SBUDDY 0x10000000
#define EBUDDY 0x20000000
#define BUDDY_BASE SBUDDY

#define FRAME_ARRAY_SIZE ((EBUDDY - SBUDDY) / PAGE_SIZE)

/*
 * Convert frame idx to the frame address
 */
static inline void *idx2addr(int idx)
{
    return (void *)((uint64)BUDDY_BASE + idx * PAGE_SIZE);
}

/*
 * Convert frame address to the frame idx
 */
static inline int addr2idx(void *hdr)
{
    return ((char *)hdr - (char *)BUDDY_BASE) / PAGE_SIZE;
}

void page_allocator_init(void);

/*
 * Allocate @num contiguous pages.
 * Return NULL if failed.
 */
void *alloc_pages(int num);

/*
 * Allocate 1 page.
 * Return NULL if failed.
 */
void *alloc_page(void);

void free_page(void *page);

#ifdef DEBUG
void page_allocator_test(void);
#endif

#endif /* _PAGE_ALLOC_H */