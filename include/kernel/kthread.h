#ifndef _KTHREAD_H
#define _KTHREAD_H

void kthread_set_init(void);

void kthread_create(void (*start)(void));

#endif /* _KTHREAD_H */