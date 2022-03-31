#ifndef _UTILS_H
#define _UTILS_H

#include <types.h>

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

void delay(uint64 cnt);
void put32(uint64 addr, uint32 val);
uint32 get32(uint64 addr);

#define enable_interrupt() do {       \
    asm volatile("msr DAIFClr, 0xf"); \
} while (0)

#define disable_interrupt() do {      \
    asm volatile("msr DAIFSet, 0xf"); \
} while (0)

#endif  /* _UTILS_H */