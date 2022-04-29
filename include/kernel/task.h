#ifndef _TASK_H
#define _TASK_H

#include <types.h>
#include <list.h>

#define STACK_SIZE (2 * PAGE_SIZE)

/* Task status */
#define TASK_NEW        0
#define TASK_RUNNING    1
#define TASK_DEAD       2

/* Define in include/kernel/signal.h */
struct signal_head_t;
struct sighand_t;

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
    uint32 datalen;
    /* @list is used by run_queue / wait_queue */
    struct list_head list;
    /* @task_list links all tasks */
    struct list_head task_list;
    uint16 status;
    uint16 need_resched:1;
    uint32 tid;
    uint32 preempt;
    /* Signal */
    struct signal_head_t *signal;
    struct sighand_t *sighand;
} task_struct;

#define SAVE_REGS(task) \
    asm volatile (                          \
        "stp x19, x20, [%x0, 16 * 0]\n"     \
        "stp x21, x22, [%x0, 16 * 1]\n"     \
        "stp x23, x24, [%x0, 16 * 2]\n"     \
        "stp x25, x26, [%x0, 16 * 3]\n"     \
        "stp x27, x28, [%x0, 16 * 4]\n"     \
        "stp fp, lr, [%x0, 16 * 5]\n"       \
        "mov x9, sp\n"                      \
        "str x9, [%x0, 16 * 6]\n"           \
        : : "r" (&task->regs)               \
    );

void task_init(void);

task_struct *task_create(void);
void task_free(task_struct *task);

task_struct *task_get_by_tid(uint32 tid);

#endif /* _TASK_H */