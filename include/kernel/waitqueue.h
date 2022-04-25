#ifndef _WAITQUEUE_H
#define _WAITQUEUE_H

#include <task.h>

typedef struct {
    struct list_head list;
} wait_queue_head;

wait_queue_head *wq_create(void);

int wq_empty(wait_queue_head *head);

void wq_add_task(task_struct *task, wait_queue_head *head);
void wq_del_task(task_struct *task);

task_struct *wq_get_first_task(wait_queue_head *head);

#endif /* _WAITQUEUE_H */