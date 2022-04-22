#ifndef _TIMER_H
#define _TIMER_H

#include <types.h>

void timer_init();
int timer_irq_check();
void timer_switch_info();

/* Call @proc(@args) after @after seconds. */
void timer_add_proc_after(void (*proc)(void *), void *args, uint32 after);

/* Call @proc(@args) after 1/@freq second. */
void timer_add_proc_freq(void (*proc)(void *), void *args, uint32 freq);

#endif /* _TIMER_H */