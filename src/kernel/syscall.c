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
#include <sched.h>
#include <signal.h>
#include <mm/mm.h>
#include <panic.h>

#define KSTACK_VARIABLE(x)                      \
    (void *)((uint64)x -                        \
             (uint64)current->kernel_stack +    \
             (uint64)child->kernel_stack)

typedef void (*syscall_funcp)();

void syscall_getpid(trapframe *_);
void syscall_uart_read(trapframe *_, char buf[], size_t size);
void syscall_uart_write(trapframe *_, const char buf[], size_t size);
void syscall_exec(trapframe *_, const char* name, char *const argv[]);
void syscall_fork(trapframe *_);
void syscall_exit(trapframe *_);
void syscall_mbox_call(trapframe *_, unsigned char ch, unsigned int *mbox);
void syscall_kill_pid(trapframe *_, int pid);
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
    (syscall_funcp) syscall_sigreturn,
};

typedef struct {
    unsigned int iss:25, // Instruction specific syndrome
                 il:1,   // Instruction length bit
                 ec:6;   // Exception class
} esr_el1;

static void show_trapframe(trapframe *regs)
{
    uart_sync_printf("\r\n[*] Trapframe:\r\n");
    uart_sync_printf("\tx0: %llx\r\n", regs->x0);
    uart_sync_printf("\tx1: %llx\r\n", regs->x1);
    uart_sync_printf("\tx2: %llx\r\n", regs->x2);
    uart_sync_printf("\tx3: %llx\r\n", regs->x3);
    uart_sync_printf("\tx4: %llx\r\n", regs->x4);
    uart_sync_printf("\tx5: %llx\r\n", regs->x5);
    uart_sync_printf("\tx6: %llx\r\n", regs->x6);
    uart_sync_printf("\tx7: %llx\r\n", regs->x7);
    uart_sync_printf("\tx8: %llx\r\n", regs->x8);
    uart_sync_printf("\tx9: %llx\r\n", regs->x9);
    uart_sync_printf("\tx10: %llx\r\n", regs->x10);
    uart_sync_printf("\tx11: %llx\r\n", regs->x11);
    uart_sync_printf("\tx12: %llx\r\n", regs->x12);
    uart_sync_printf("\tx13: %llx\r\n", regs->x13);
    uart_sync_printf("\tx14: %llx\r\n", regs->x14);
    uart_sync_printf("\tx15: %llx\r\n", regs->x15);
    uart_sync_printf("\tx16: %llx\r\n", regs->x16);
    uart_sync_printf("\tx17: %llx\r\n", regs->x17);
    uart_sync_printf("\tx18: %llx\r\n", regs->x18);
    uart_sync_printf("\tx19: %llx\r\n", regs->x19);
    uart_sync_printf("\tx20: %llx\r\n", regs->x20);
    uart_sync_printf("\tx21: %llx\r\n", regs->x21);
    uart_sync_printf("\tx22: %llx\r\n", regs->x22);
    uart_sync_printf("\tx23: %llx\r\n", regs->x23);
    uart_sync_printf("\tx24: %llx\r\n", regs->x24);
    uart_sync_printf("\tx25: %llx\r\n", regs->x25);
    uart_sync_printf("\tx26: %llx\r\n", regs->x26);
    uart_sync_printf("\tx27: %llx\r\n", regs->x27);
    uart_sync_printf("\tx28: %llx\r\n", regs->x28);
    uart_sync_printf("\tx29: %llx\r\n", regs->x29);
    uart_sync_printf("\tx30: %llx\r\n", regs->x30);
    uart_sync_printf("\tsp_el0  : %llx\r\n", regs->sp_el0);
    uart_sync_printf("\telr_el1 : %llx\r\n", regs->elr_el1);
    uart_sync_printf("\tspsr_el1: %llx\r\n", regs->spsr_el1);
}

void syscall_handler(trapframe regs, uint32 syn)
{
    esr_el1 *esr;
    uint64 syscall_num;
      
    esr = (esr_el1 *)&syn;

    // SVC instruction execution
    if (esr->ec != 0x15) {
        show_trapframe(&regs);
        panic("[X] Panic: esr->ec: %x", esr->ec);
        // Never return
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
    uint32 datalen;
    
    datalen = cpio_load_prog(initramfs_base, name, (char **)&data);

    if (datalen == 0) {
        return;
    }

    // Use origin kernel stack

    // TODO: Clear user stack

    kfree(current->data);
    current->data = data;
    current->datalen = datalen;

    kernel_sp = (char *)current->kernel_stack + STACK_SIZE - 0x10;

    // Reset signal
    signal_head_reset(current->signal);
    sighand_reset(current->sighand);

    // Reset page table
    pt_free(current->page_table);
    current->page_table = pt_create();
    task_init_map(current);

    // 0x000000000000 ~ <datalen>: rwx: Code
    pt_map(current->page_table, (void *)0, datalen,
           (void *)VA2PA(data), PT_R | PT_W | PT_X);

    set_page_table(current);

    exec_user_prog((void *)0, (char *)0xffffffffeff0, kernel_sp);
}

static inline void copy_regs(struct pt_regs *regs)
{
    regs->x19 = current->regs.x19;
    regs->x20 = current->regs.x20;
    regs->x21 = current->regs.x21;
    regs->x22 = current->regs.x22;
    regs->x23 = current->regs.x23;
    regs->x24 = current->regs.x24;
    regs->x25 = current->regs.x25;
    regs->x26 = current->regs.x26;
    regs->x27 = current->regs.x27;
    regs->x28 = current->regs.x28;
}

void syscall_fork(trapframe *frame)
{
    task_struct *child;
    trapframe *child_frame;

    child = task_create();

    child->kernel_stack = kmalloc(STACK_SIZE);
    child->user_stack = kmalloc(STACK_SIZE);    
    child->data = kmalloc(current->datalen);
    child->datalen = current->datalen;

    memncpy(child->kernel_stack, current->kernel_stack, STACK_SIZE);
    memncpy(child->user_stack, current->user_stack, STACK_SIZE);
    memncpy(child->data, current->data, current->datalen);

    // Set page table
    task_init_map(child);

    // 0x000000000000 ~ <datalen>: rwx: Code
    pt_map(child->page_table, (void *)0, child->datalen,
           (void *)VA2PA(child->data), PT_R | PT_W | PT_X);

    // Copy signal handler
    sighand_copy(child->sighand, child->data);

    // Save registers
    SAVE_REGS(current);

    // Copy registers
    copy_regs(&child->regs);

    // Copy stack related registers
    child->regs.fp = KSTACK_VARIABLE(current->regs.fp);
    child->regs.sp = KSTACK_VARIABLE(current->regs.sp);

    // https://gcc.gnu.org/onlinedocs/gcc/Labels-as-Values.html
    child->regs.lr = &&SYSCALL_FORK_END;

    // Adjust child trapframe
    child_frame = KSTACK_VARIABLE(frame);

    child_frame->x0 = 0;

    sched_add_task(child);

    // Set return value
    frame->x0 = child->tid;

SYSCALL_FORK_END:

    asm volatile("nop");
}

void syscall_exit(trapframe *_)
{
    exit_user_prog();

    // Never reach
}

/*
 * TODO: map the return addres of mailbox_call
 */
void syscall_mbox_call(trapframe *_, unsigned char ch, unsigned int *mbox)
{
    int mbox_size;
    char *kmbox;

    mbox_size = (int)mbox[0];

    if (mbox_size <= 0)
        return;

    kmbox = kmalloc(mbox_size);

    memncpy(kmbox, (char *)mbox, mbox_size);

    mailbox_call(ch, (unsigned int *)kmbox);

    memncpy((char *)mbox, kmbox, mbox_size);

    kfree(kmbox);
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