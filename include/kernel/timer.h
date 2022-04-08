#ifndef _TIMER_H
#define _TIMER_H

#include <types.h>

void timer_init();
void timer_irq_check();
void timer_irq_handler();
void timer_switch_info();

/* Call @proc(@args) after @after seconds. */
void timer_add_proc(void (*proc)(void *), void *args, uint32 after);

#endif /* _TIMER_H */