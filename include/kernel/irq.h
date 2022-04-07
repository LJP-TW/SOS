#ifndef _IRQ_H
#define _IRQ_H

#include <types.h>

void irq_init();
void irq_add_tasks(void (*task)(void *), void *args, uint32 prio);

void irq_handler();
void exception_default_handler(uint32 n);
void irq1_enable(int bit);

#endif /* _IRQ_H */