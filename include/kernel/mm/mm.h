#ifndef _MM_H
#define _MM_H

#include <mm/early_alloc.h>
#include <mm/page_alloc.h>
#include <mm/sc_alloc.h>

void mm_init(void);

void *kmalloc(int size);

void kfree(void *ptr);

#endif /* _MM_H */