#include <task.h>
#include <mm/mm.h>

// TODO: recycle usable tid
uint32 max_tid;

static uint32 alloc_tid(void)
{
    uint32 tid;

    tid = max_tid;
    max_tid += 1;

    return tid;
}

task_struct *task_create(void)
{
    task_struct *task;
    
    task = kmalloc(sizeof(task_struct));

    task->kernel_stack = NULL;
    task->user_stack = NULL;
    task->data = NULL;
    task->need_resched = 0;
    task->tid = alloc_tid();
    task->preempt = 0;

    return task;
}

void task_free(task_struct *task)
{
    if (task->kernel_stack)
        kfree(task->kernel_stack);

    if (task->user_stack)
        kfree(task->user_stack);

    if (task->data)
        kfree(task->data);

    // TODO: release tid
    
    kfree(task);
}