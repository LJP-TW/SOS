#ifndef _ENTRY_H
#define _ENTRY_H

#include <types.h>
#include <trapframe.h>

void el0_sync_handler(trapframe *regs, uint32 syn);

#endif /* _ENTRY_H */