#include <waitqueue.h>
#include <preempt.h>
#include <mm/mm.h>

wait_queue_head *wq_create(void)
{
    wait_queue_head *head;

    head = kmalloc(sizeof(wait_queue_head));

    INIT_LIST_HEAD(&head->list);

    return head;
}

int wq_empty(wait_queue_head *head)
{
    return list_empty(&head->list);
}

void wq_add_task(task_struct *task, wait_queue_head *head)
{
    preempt_disable();

    list_add_tail(&task->list, &head->list);

    preempt_enable();
}

void wq_del_task(task_struct *task)
{
    preempt_disable();

    list_del(&task->list);

    preempt_enable();
}

task_struct *wq_get_first_task(wait_queue_head *head)
{
    task_struct *task;

    preempt_disable();

    task = list_first_entry(&head->list, task_struct, list);

    preempt_enable();

    return task;
}