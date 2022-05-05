#ifndef _SIGNAL_H
#define _SIGNAL_H

#include <types.h>
#include <list.h>
#include <trapframe.h>

// https://man7.org/linux/man-pages/man7/signal.7.html
// 0 means no signal
#define SIGHUP      1
#define SIGINT      2
#define SIGQUIT     3
#define SIGILL      4
#define SIGTRAP     5
#define SIGABRT     6
#define SIGIOT      6
#define SIGBUS      7
#define SIGFPE      8
#define SIGKILL     9
#define SIGUSR1     10
#define SIGSEGV     11
#define SIGUSR2     12
#define SIGPIPE     13
#define SIGALRM     14
#define SIGTERM     15
#define SIGSTKFLT   16
#define SIGCHLD     17
#define SIGCONT     18
#define SIGSTOP     19
#define SIGTSTP     20
#define SIGTTIN     21
#define SIGTTOU     22
#define SIGURG      23
#define SIGXCPU     24
#define SIGXFSZ     25
#define SIGVTALRM   26
#define SIGPROF     27
#define SIGWINCH    28
#define SIGIO       29
#define SIGPOLL     29
#define SIGPWR      30
#define SIGSYS      31
#define SIGUNUSED   31

#define NSIG		32

struct signal_head_t {
    /* Link signal_t */
    struct list_head list;
};

struct signal_t {
    struct list_head list;
    uint32 signum;
};

typedef void (*sighandler_t)(int);

struct sigaction_t {
    sighandler_t sighand;

    /* flags */
    uint32 kernel_hand;
};

struct sighand_t {
    // 0-th sigaction_t is not used
    struct sigaction_t sigactions[NSIG];
};

struct signal_head_t *signal_head_create(void);
void signal_head_free(struct signal_head_t *head);
void signal_head_reset(struct signal_head_t *head);

void handle_signal(trapframe *_);

struct sighand_t *sighand_create(void);
void sighand_free(struct sighand_t *sighand);
void sighand_reset(struct sighand_t *sighand);

/* Copy current signal handler to @sighand */
void sighand_copy(struct sighand_t *sighand, void *addrbase);

/* syscalls */
void syscall_signal(trapframe *_, uint32 signal, void (*handler)(int));
void syscall_kill(trapframe *_, int pid, int signal);
void syscall_sigreturn(trapframe *_);

#endif /* _SIGNAL_H */