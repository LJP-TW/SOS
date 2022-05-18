#include <exec.h>
#include <task.h>
#include <cpio.h>
#include <current.h>
#include <sched.h>
#include <kthread.h>
#include <mm/mm.h>

// Change current EL to EL0 and execute the user program at @entry
// Set user stack to @user_sp
void enter_el0_run_user_prog(void *entry, char *user_sp);

static void user_prog_start(void)
{
    enter_el0_run_user_prog((void *)0, (char *)0xffffffffeff0);

    // User program should call exit() to terminate
}

static inline void pt_regs_init(struct pt_regs *regs)
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
    regs->lr = user_prog_start;
}

// TODO: Add argv & envp
void sched_new_user_prog(char *filename)
{
    void *data;
    uint32 datalen;
    task_struct *task;
    
    datalen = cpio_load_prog(initramfs_base, filename, (char **)&data);

    if (datalen == 0) {
        goto EXEC_USER_PROG_END;
    }

    task = task_create();

    task->kernel_stack = kmalloc(STACK_SIZE);
    task->user_stack = kmalloc(STACK_SIZE);
    task->data = data;
    task->datalen = datalen;

    task->regs.sp = (char *)task->kernel_stack + STACK_SIZE - 0x10;
    pt_regs_init(&task->regs);

    task_init_map(task);

    // 0x000000000000 ~ <datalen>: rwx: Code
    pt_map(task->page_table, (void *)0, datalen,
           (void *)VA2PA(task->data), PT_R | PT_W | PT_X);

    sched_add_task(task);

EXEC_USER_PROG_END:
    return;
}

void exit_user_prog(void)
{
    kthread_fini();

    // Never reach
}