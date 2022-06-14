#ifndef _SYSCALL_H
#define _SYSCALL_H

#include <trapframe.h>

void syscall_handler(trapframe *regs);

#endif /* _SYSCALL_H */