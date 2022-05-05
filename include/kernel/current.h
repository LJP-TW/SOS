#ifndef _CURRENT_H
#define _CURRENT_H

#include <utils.h>

struct _task_struct;

static inline struct _task_struct *get_current(void)
{
    return (struct _task_struct *)read_sysreg(tpidr_el1);
}

static inline void set_current(struct _task_struct *task)
{
    write_sysreg(tpidr_el1, task);
}

#define current get_current()

#endif /* _CURRENT_H */