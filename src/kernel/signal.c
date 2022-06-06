#include <signal.h>
#include <task.h>
#include <exec.h>
#include <current.h>
#include <preempt.h>
#include <mm/mm.h>
#include <text_user_shared.h>
#include <syscall.h>

// TODO: implement SIGSTOP & SIGCONT kernel handler

/* Kernel defined sighandler_t */
#define SIG_DFL     (sighandler_t)0
#define SIG_IGN     (sighandler_t)1

// SIG_DFL
static void sig_termiante(int);

// SIG_IGN
static void sig_ignore(int);

static void sig_termiante(int _)
{
    exit_user_prog();

    // Never reach
}

static void sig_ignore(int _)
{
    return;
}

void sigreturn(void) SECTION_TUS;
void sigreturn(void)
{
    asm volatile(
        "mov x8, " MC2STR(SCNUM_SIGRETURN) "\n"
        "svc 0\n"
    );
}

/* 
 * Try to get the pending signal of current task.
 */
static struct signal_t *signal_try_get(void)
{
    if (list_empty(&current->signal->list)) {
       return NULL; 
    }

    return list_first_entry(&current->signal->list, struct signal_t, list);
}

static void signal_add(uint32 signum, struct signal_head_t *head)
{
    struct signal_t *signal;

    signal = kmalloc(sizeof(struct signal_t));

    signal->signum = signum;

    list_add(&signal->list, &head->list);
}

static void signal_del(struct signal_t *signal)
{
    list_del(&signal->list);
    kfree(signal);
}

static void save_context(void *user_sp, trapframe *frame)
{
    memncpy(user_sp, (char *)frame, sizeof(trapframe));
}

struct signal_head_t *signal_head_create(void)
{
    struct signal_head_t *head;

    head = kmalloc(sizeof(struct signal_head_t));

    INIT_LIST_HEAD(&head->list);

    return head;
}

void signal_head_free(struct signal_head_t *head)
{
    struct signal_t *signal, *safe;

    list_for_each_entry_safe(signal, safe, &head->list, list) {
        signal_del(signal);
    }

    kfree(head);
}

void signal_head_reset(struct signal_head_t *head)
{
    struct signal_t *signal, *safe;

    list_for_each_entry_safe(signal, safe, &head->list, list) {
        signal_del(signal);
    }
}

void handle_signal(trapframe *frame)
{
    struct signal_t *signal;
    struct sigaction_t *sigaction;

    preempt_disable();

    signal = signal_try_get();

    preempt_enable();

    if (signal == NULL) {
        return;
    }

    sigaction = &current->sighand->sigactions[signal->signum];

    if (sigaction->kernel_hand) {
        sigaction->sighand(signal->signum);
    } else {
        char *user_sp;
        uint32 reserve_size;

        // Reserve space on user stack
        reserve_size = sizeof(trapframe);
        user_sp = frame->sp_el0 - ALIGN(reserve_size, 0x10);

        // Save cpu context onto user stack
        save_context(user_sp, frame);

        // Set user sp
        frame->sp_el0 = user_sp;

        // Set parameter of handler
        frame->x0 = signal->signum;

        // Set user pc to handler
        frame->elr_el1 = sigaction->sighand;

        // Set user lr to sigreturn
        frame->x30 = TUS2VA(sigreturn);
    }

    signal_del(signal);
}

struct sighand_t *sighand_create(void)
{
    struct sighand_t *sighand;

    sighand = kmalloc(sizeof(struct sighand_t));

    for (int i = 1; i < NSIG; ++i) {
        sighand->sigactions[i].kernel_hand = 1;
        sighand->sigactions[i].sighand = sig_ignore;
    }

    sighand->sigactions[SIGKILL].sighand = sig_termiante;

    return sighand;
}

void sighand_free(struct sighand_t *sighand)
{
    kfree(sighand);
}

void sighand_reset(struct sighand_t *sighand)
{
    for (int i = 1; i < NSIG; ++i) {
        sighand->sigactions[i].kernel_hand = 1;
        sighand->sigactions[i].sighand = sig_ignore;
    }

    sighand->sigactions[SIGKILL].sighand = sig_termiante;
}

static inline void _sighand_copy(struct sigaction_t *to,
                                struct sigaction_t *from)
{
    to->kernel_hand = from->kernel_hand;
    to->sighand = from->sighand;
}

/* Copy current signal handler to @sighand */
void sighand_copy(struct sighand_t *sighand)
{
    struct sighand_t *currhand;

    currhand = current->sighand;

    for (int i = 1; i < NSIG; ++i) {
        _sighand_copy(&sighand->sigactions[i], &currhand->sigactions[i]);
    }
}

void syscall_signal(trapframe *_, uint32 signal, void (*handler)(int))
{
    struct sigaction_t *sigaction;

    // Check if signal is valid
    if (signal >= NSIG) {
        return;
    }

    sigaction = &current->sighand->sigactions[signal];

    if (handler == SIG_DFL) {
        sigaction->kernel_hand = 1;
        sigaction->sighand = sig_termiante;
    } else if (handler == SIG_IGN) {
        sigaction->kernel_hand = 1;
        sigaction->sighand = sig_ignore;
    } else {
        sigaction->kernel_hand = 0;
        sigaction->sighand = handler;
    }
}

void syscall_kill(trapframe *_, int pid, int signal)
{
    task_struct *task;
    
    preempt_disable();

    task = task_get_by_tid(pid);

    if (!task || task->status != TASK_RUNNING) {
        goto SYSCALL_KILL_END;
    }

    signal_add((uint32)signal, task->signal);

SYSCALL_KILL_END:
    preempt_enable();
}

// restore context
void syscall_sigreturn(trapframe *frame)
{
    trapframe *user_sp;

    user_sp = frame->sp_el0;

    memncpy((char *)frame, (char *)user_sp, sizeof(trapframe));
}