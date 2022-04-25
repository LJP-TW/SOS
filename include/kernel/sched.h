#ifndef _SCHED_H
#define _SCHED_H

#include <task.h>

void switch_to(task_struct *from, task_struct *to);

void scheduler_init(void);

void schedule(void);

void schedule_tick(void);

void sched_add_task(task_struct *task);

void sched_del_task(task_struct *task);

#endif /* _SCHED_H */