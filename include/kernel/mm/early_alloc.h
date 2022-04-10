#ifndef _MEM_H
#define _MEM_H

#include <types.h>

/* From linker.ld */
extern char _early_mem_base;
extern char _early_mem_end;

#define EARLY_MEM_BASE (&_early_mem_base)
#define EARLY_MEM_END  (&_early_mem_end)

void* early_malloc(size_t size);

#endif /* _MEM_H */