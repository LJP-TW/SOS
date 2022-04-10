#ifndef _PAGE_ALLOC_H
#define _PAGE_ALLOC_H

#include <types.h>

/* Initialize by page_allocator_early_init() */
extern uint32 frame_ents_size;
extern uint64 buddy_base;
extern uint64 buddy_end;

/*
 * Convert frame idx to the frame address
 */
static inline void *idx2addr(int idx)
{
    return (void *)(buddy_base + idx * PAGE_SIZE);
}

/*
 * Convert frame address to the frame idx
 */
static inline int addr2idx(void *hdr)
{
    return ((uint64)hdr - buddy_base) / PAGE_SIZE;
}

/*
 * Initialize @start ~ @end memory for buddy system
 * page_allocator_early_init() must be called before calling mem_reserve().
 */
void page_allocator_early_init(void *start, void *end);

/*
 * Reserve @start ~ @end memory.
 * mem_reserve() must be called before calling page_allocator_init().
 */
void mem_reserve(void *start, void *end);

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