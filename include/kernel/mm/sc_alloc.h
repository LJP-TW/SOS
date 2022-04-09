#ifndef _SC_ALLOC_H
#define _SC_ALLOC_H

#include <types.h>

void sc_init(void);

void *sc_alloc(int size);

void sc_free(void *sc);

#ifdef DEBUG
void sc_test(void);
#endif

#endif /* _SC_ALLOC_H */