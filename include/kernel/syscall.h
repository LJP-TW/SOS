#ifndef _SYSCALL_H
#define _SYSCALL_H

#include <types.h>

// @syn: syndrome information for an synchronous exception
// One should pass esr_el1 register value to @syn
void syscall_handler(uint32 syn);

#endif /* _SYSCALL_H */