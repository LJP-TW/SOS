#include <kthread.h>
#include <current.h>
#include <sched.h>
#include <mm/mm.h>

#define STACK_SIZE (2 * PAGE_SIZE)

// TODO: recycle usable tid
uint32 max_tid;

static uint32 alloc_tid(void)
{
    uint32 tid;

    tid = max_tid;
    max_tid += 1;

    return tid;
}

void kthread_set_init(void)
{
    task_struct *task;
    
    task = kmalloc(sizeof(task_struct));

    task->need_resched = 0;
    task->tid = alloc_tid();

    sched_add_task(task);

    set_current(task);
}

static inline void pt_regs_init(struct pt_regs *regs, void *lr)
{
    regs->x19 = 0;
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
    regs->lr = lr;
}

void kthread_create(void (*start)(void))
{
    task_struct *task;
    
    task = kmalloc(sizeof(task_struct));

    task->kernel_stack = kmalloc(STACK_SIZE);
    task->need_resched = 0;
    task->tid = alloc_tid();
    task->regs.sp = (char *)task->kernel_stack + STACK_SIZE - 0x10;
    pt_regs_init(&task->regs, start);

    sched_add_task(task);
}