#ifndef _IRQ_H
#define _IRQ_H

#include <types.h>

void irq_init();

/*
 * On success, 0 is returned
 */
int irq_run_task(void (*task)(void *),
                 void *args,
                 void (*fini)(void),
                 uint32 prio);

void irq_handler();
void exception_default_handler(uint32 n);
void irq1_enable(int bit);

#endif /* _IRQ_H */