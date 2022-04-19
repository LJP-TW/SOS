#ifndef _BITOPS_H
#define _BITOPS_H

// Find First bit Set
#define ffs(x) __builtin_ffs(x)

static inline int fls(unsigned int x)
{
    return x ? sizeof(x) * 8 - __builtin_clz(x) : 0;
}

#endif