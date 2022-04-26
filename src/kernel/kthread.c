#include <kthread.h>
#include <current.h>
#include <sched.h>
#include <mm/mm.h>
#include <waitqueue.h>
#include <preempt.h>

#define STACK_SIZE (2 * PAGE_SIZE)

static wait_queue_head *wait_queue;

static void kthread_start(void)
{
    void (*main)(void);

    asm volatile("mov %0, x19\n"
                 "mov x19, xzr"
                 : "=r" (main));

    main();

    kthread_fini();
}

void kthread_add_wait_queue(task_struct *task)
{
    wq_add_task(task, wait_queue);
}

/*
 * Add to wait_queue to wait some process to recycle this kthread
 */
void kthread_fini(void)
{    
    preempt_disable();

    current->status = TASK_DEAD;

    sched_del_task(current);

    wq_add_task(current, wait_queue);

    preempt_enable();

    schedule();

    // Never reach
}

void kthread_init(void)
{
    task_struct *task;
    
    // Initialze current task
    task = task_create();

    // Must set current first
    set_current(task);

    sched_add_task(task);

    // Create wait_queue
    wait_queue = wq_create();
}

static inline void pt_regs_init(struct pt_regs *regs, void *main)
{
    regs->x19 = main;
    regs->x20 = 0;
    regs->x21 = 0;
    regs->x22 = 0;
    regs->x23 = 0;
    regs->x24 = 0;
    regs->x25 = 0;
    regs->x26 = 0;
    regs->x27 = 0;
    regs->x28 = 0;
    regs->fp = 0;
    regs->lr = kthread_start;
}

void kthread_create(void (*start)(void))
{
    task_struct *task;
    
    task = task_create();

    task->kernel_stack = kmalloc(STACK_SIZE);
    task->regs.sp = (char *)task->kernel_stack + STACK_SIZE - 0x10;
    pt_regs_init(&task->regs, start);

    sched_add_task(task);
}

void kthread_kill_zombies(void)
{
    while (1) {
        task_struct *task;

        if (wq_empty(wait_queue)) {
            return;
        }

        preempt_disable();
        
        task = wq_get_first_task(wait_queue);

        wq_del_task(task);

        preempt_enable();

        task_free(task);
    }
}