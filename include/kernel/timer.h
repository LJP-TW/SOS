#ifndef _TIMER_H
#define _TIMER_H

#include <types.h>

void timer_init();
void timer_irq_handler();
void timer_switch_info();

#endif /* _TIMER_H */