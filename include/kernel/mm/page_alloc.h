#ifndef _PAGE_ALLOC_H
#define _PAGE_ALLOC_H

#include <types.h>

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