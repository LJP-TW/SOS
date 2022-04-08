#ifndef _UTILS_H
#define _UTILS_H

#include <types.h>

#ifndef __ASSEMBLER__

void delay(uint64 cnt);
void put32(uint64 addr, uint32 val);
uint32 get32(uint64 addr);

void memzero(char *src, unsigned long n);
void memncpy(char *dst, char *src, unsigned long n);

#endif /* __ASSEMBLER__ */

// Reference from https://elixir.bootlin.com/linux/latest/source/tools/lib/perf/mmap.c#L299
#define read_sysreg(r) ({                       \
    uint64 __val;                               \
    asm volatile("mrs %0, " #r : "=r" (__val)); \
    __val;                                      \
})

// Reference from https://elixir.bootlin.com/linux/latest/source/arch/arm64/include/asm/sysreg.h#L1281
#define write_sysreg(r, v) do {    \
    uint64 __val = (uint64)(v);          \
    asm volatile("msr " #r ", %x0" \
             : : "rZ" (__val));    \
} while (0)

#define enable_interrupt() do {       \
    asm volatile("msr DAIFClr, 0xf"); \
} while (0)

#define disable_interrupt() do {      \
    asm volatile("msr DAIFSet, 0xf"); \
} while (0)

#define get_elem_idx(elem, array) \
    (((char *)elem - (char *)array) / sizeof(array[0]))

#define ALIGN(num, base) ((num + base - 1) & ~(base - 1))

#define TO_CHAR_PTR(a) ((char *)(uint64)(a))

#endif  /* _UTILS_H */