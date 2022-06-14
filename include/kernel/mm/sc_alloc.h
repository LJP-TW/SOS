#ifndef _SC_ALLOC_H
#define _SC_ALLOC_H

#include <types.h>

void sc_early_init(void);

void sc_init(void);

void *sc_alloc(int size);

/*
 * Return 0 if successful.
 */
int sc_free(void *sc);

#ifdef MM_DEBUG
void sc_test(void);
#endif

#endif /* _SC_ALLOC_H */