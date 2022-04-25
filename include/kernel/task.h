#ifndef _TASK_H
#define _TASK_H

#include <types.h>
#include <list.h>

struct pt_regs {
    void *x19;
    void *x20;
    void *x21;
    void *x22;
    void *x23;
    void *x24;
    void *x25;
    void *x26;
    void *x27;
    void *x28;
    void *fp;
    void *lr;
    void *sp;
};

typedef struct _task_struct {
    /* This must be the first element */
    struct pt_regs regs;
    void *kernel_stack;
    void *user_stack;
    /* TODO: Update to address_space */
    void *data;
    struct list_head list;
    uint32 need_resched:1;
    uint32 tid;
    uint32 preempt;
} task_struct;

task_struct *task_create(void);
void task_free(task_struct *task);

#endif /* _TASK_H */