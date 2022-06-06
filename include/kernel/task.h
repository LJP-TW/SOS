#ifndef _TASK_H
#define _TASK_H

#include <types.h>
#include <list.h>
#include <mmu.h>
#include <fs/vfs.h>

#define STACK_SIZE (4 * PAGE_SIZE)

#define TASK_MAX_FD     0x10

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
    struct pt_regs regs;
    pd_t *page_table;
    /* The order of the above elements cannot be changed */
    vm_area_meta_t *address_space;
    void *kernel_stack;
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
    /* Files */
    int maxfd;
    struct file fds[TASK_MAX_FD];
    struct vnode *work_dir;
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

/*
 * Create initial mapping for user program
 *
 * 0x00003c000000 ~ 0x00003f000000: rw-: Mailbox address
 * 0x7f0000000000 ~   <shared_len>: r-x: Kernel functions exposed to users
 * 0xffffffffb000 ~   <STACK_SIZE>: rw-: Stack
 */
void task_init_map(task_struct *task);

void task_reset_mm(task_struct *task);

#endif /* _TASK_H */