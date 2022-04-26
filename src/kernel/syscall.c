#include <mini_uart.h>
#include <syscall.h>
#include <utils.h>
#include <exec.h>
#include <current.h>
#include <task.h>
#include <mini_uart.h>
#include <rpi3.h>
#include <preempt.h>
#include <kthread.h>
#include <cpio.h>
#include <mm/mm.h>

typedef void (*syscall_funcp)();

void syscall_getpid(trapframe *_);
void syscall_uart_read(trapframe *_, char buf[], size_t size);
void syscall_uart_write(trapframe *_, const char buf[], size_t size);
void syscall_exec(trapframe *_, const char* name, char *const argv[]);
void syscall_fork(trapframe *_);
void syscall_exit(trapframe *_);
void syscall_mbox_call(trapframe *_, unsigned char ch, unsigned int *mbox);
void syscall_kill_pid(trapframe *_, int pid);
void syscall_signal(trapframe *_, int signal, void (*handler)(void));
void syscall_kill(trapframe *_, int pid, int signal);
void syscall_show_info(trapframe *_);

syscall_funcp syscall_table[] = {
    (syscall_funcp) syscall_getpid,     // 0
    (syscall_funcp) syscall_uart_read,
    (syscall_funcp) syscall_uart_write,
    (syscall_funcp) syscall_exec,
    (syscall_funcp) syscall_fork,       // 4
    (syscall_funcp) syscall_exit,
    (syscall_funcp) syscall_mbox_call,
    (syscall_funcp) syscall_kill_pid,
    (syscall_funcp) syscall_signal,     // 8
    (syscall_funcp) syscall_kill,
    (syscall_funcp) syscall_show_info,
};

typedef struct {
    unsigned int iss:25, // Instruction specific syndrome
                 il:1,   // Instruction length bit
                 ec:6;   // Exception class
} esr_el1;

void syscall_handler(trapframe regs, uint32 syn)
{
    esr_el1 *esr;
    uint64 syscall_num;
      
    esr = (esr_el1 *)&syn;

    // SVC instruction execution
    if (esr->ec != 0x15) {
        return;
    }

    syscall_num = regs.x8;

    if (syscall_num > ARRAY_SIZE(syscall_table)) {
        // Invalid syscall
        return;
    }

    enable_interrupt();

    (syscall_table[syscall_num])(&regs,
        regs.x0, regs.x1, regs.x2, regs.x3, regs.x4, regs.x5);

    disable_interrupt();
}

void syscall_getpid(trapframe *frame)
{
    frame->x0 = current->tid;
}

void syscall_uart_read(trapframe *_, char buf[], size_t size)
{
    uart_recvn(buf, size);
}

void syscall_uart_write(trapframe *_, const char buf[], size_t size)
{
    uart_sendn(buf, size);
}

// TODO: Passing argv
void syscall_exec(trapframe *_, const char* name, char *const argv[])
{
    void *data;
    char *kernel_sp;
    char *user_sp;
    
    data = cpio_load_prog(initramfs_base, name);

    if (data == NULL) {
        return;
    }

    // Use origin kernel stack

    // TODO: Clear user stack

    kfree(current->data);
    current->data = data;

    kernel_sp = (char *)current->kernel_stack + STACK_SIZE - 0x10;
    user_sp = (char *)current->user_stack + STACK_SIZE - 0x10;

    exec_user_prog(current->data, user_sp, kernel_sp);
}

void syscall_fork(trapframe *_)
{
    // TODO
}

void syscall_exit(trapframe *_)
{
    exit_user_prog();

    // Never reach
}

void syscall_mbox_call(trapframe *_, unsigned char ch, unsigned int *mbox)
{
    mailbox_call(ch, mbox);
}

void syscall_kill_pid(trapframe *_, int pid)
{
    task_struct *task;

    if (current->tid == pid) {
        exit_user_prog();

        // Never reach
        return;
    }

    preempt_disable();

    task = task_get_by_tid(pid);

    if (!task || task->status != TASK_RUNNING) {
        goto SYSCALL_KILL_PID_END;
    }

    list_del(&task->list);
    kthread_add_wait_queue(task);

SYSCALL_KILL_PID_END:
    preempt_enable();
}

void syscall_signal(trapframe *_, int signal, void (*handler)(void))
{
    // TODO
}

void syscall_kill(trapframe *_, int pid, int signal)
{
    // TODO
}

// Print the content of spsr_el1, elr_el1 and esr_el1
void syscall_show_info(trapframe *_)
{
    uint64 spsr_el1;
    uint64 elr_el1;
    uint64 esr_el1;

    spsr_el1 = read_sysreg(spsr_el1);
    elr_el1 = read_sysreg(elr_el1);
    esr_el1 = read_sysreg(esr_el1);

    uart_printf("[TEST] spsr_el1: %llx; elr_el1: %llx; esr_el1: %llx\r\n",
        spsr_el1, elr_el1, esr_el1);
}