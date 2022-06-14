#include <task.h>
#include <signal.h>
#include <mm/mm.h>
#include <text_user_shared.h>
#include <utils.h>

// TODO: Use rbtree to manage tasks
static struct list_head task_queue;

// TODO: recycle usable tid
uint32 max_tid;

static uint32 alloc_tid(void)
{
    uint32 tid;

    tid = max_tid;
    max_tid += 1;

    return tid;
}

void task_init(void)
{
    INIT_LIST_HEAD(&task_queue);
}

task_struct *task_create(void)
{
    task_struct *task;
    struct signal_head_t *signal;
    struct sighand_t *sighand;
    pd_t *page_table;
    vm_area_meta_t *as;
    
    task = kmalloc(sizeof(task_struct));
    signal = signal_head_create();
    sighand = sighand_create();
    page_table = pt_create();
    as = vma_meta_create();

    task->address_space = as;
    task->kernel_stack = NULL;
    task->page_table = page_table;
    INIT_LIST_HEAD(&task->list);
    list_add_tail(&task->task_list, &task_queue);
    task->status = TASK_NEW;
    task->need_resched = 0;
    task->tid = alloc_tid();
    task->preempt = 0;

    task->signal = signal;
    task->sighand = sighand;

    task->work_dir = rootmount->root;

    vfs_open("/dev/uart", 0, &task->fds[0]);
    vfs_open("/dev/uart", 0, &task->fds[1]);
    vfs_open("/dev/uart", 0, &task->fds[2]);

    task->maxfd = 2;

    return task;
}

void task_free(task_struct *task)
{
    if (task->kernel_stack)
        kfree(task->kernel_stack);

    list_del(&task->task_list);

    signal_head_free(task->signal);
    sighand_free(task->sighand);

    vma_meta_free(task->address_space, task->page_table);
    pt_free(task->page_table);

    // TODO: release tid

    for (int i = 0; i <= task->maxfd; ++i) {
        if (task->fds[i].vnode != NULL) {
            vfs_close(&task->fds[i]);
        }
    }

    kfree(task);
}

task_struct *task_get_by_tid(uint32 tid)
{
    task_struct *task;

    list_for_each_entry(task, &task_queue, task_list) {
        if (task->tid == tid) {
            return task;
        }
    }

    return NULL;
}

void task_init_map(task_struct *task)
{
    // TODO: map the return addres of mailbox_call
    vma_map(task->address_space, (void *)0x3c000000, 0x03000000,
           VMA_R | VMA_W | VMA_PA, (void *)0x3c000000);

    vma_map(task->address_space, (void *)0x7f0000000000, TEXT_USER_SHARED_LEN,
           VMA_R | VMA_X | VMA_PA, (void *)VA2PA(TEXT_USER_SHARED_BASE));

    vma_map(task->address_space, (void *)0xffffffffb000, STACK_SIZE,
           VMA_R | VMA_W | VMA_ANON, NULL);
}

void task_reset_mm(task_struct *task)
{
    vma_meta_free(task->address_space, task->page_table);
    pt_free(task->page_table);
    
    task->page_table = pt_create();
    task->address_space = vma_meta_create();
}