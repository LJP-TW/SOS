#include <kthread.h>
#include <current.h>
#include <sched.h>
#include <mm/mm.h>
#include <waitqueue.h>
#include <preempt.h>

#define STACK_SIZE (2 * PAGE_SIZE)

static uint32 alloc_tid(void);
static void kthread_start(void);
static void kthread_fini(void);

static wait_queue_head *wait_queue;

// TODO: recycle usable tid
uint32 max_tid;

static uint32 alloc_tid(void)
{
    uint32 tid;

    tid = max_tid;
    max_tid += 1;

    return tid;
}

static void kthread_start(void)
{
    void (*main)(void);

    asm volatile("mov %0, x19\n"
                 "mov x19, xzr"
                 : "=r" (main));

    main();

    kthread_fini();
}

/*
 * Add to wait_queue to wait some process to recycle this kthread
 */
static void kthread_fini(void)
{    
    preempt_disable();

    sched_del_task(current);

    wq_add_task(current, wait_queue);

    preempt_enable();

    schedule();
}

void kthread_init(void)
{
    task_struct *task;
    
    // Initialze current task
    task = kmalloc(sizeof(task_struct));

    task->need_resched = 0;
    task->tid = alloc_tid();
    task->preempt = 0;

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
    
    task = kmalloc(sizeof(task_struct));

    task->kernel_stack = kmalloc(STACK_SIZE);
    task->need_resched = 0;
    task->tid = alloc_tid();
    task->preempt = 0;
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

        kfree(task->kernel_stack);
        // TODO: release tid
        kfree(task);
    }
}