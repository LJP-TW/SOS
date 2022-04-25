#include <mini_uart.h>
#include <syscall.h>
#include <utils.h>
#include <exec.h>

typedef void *(*syscall_funcp)();

void *syscall_getpid(void);
void *syscall_uartread(char buf[], size_t size);
void *syscall_uartwrite(const char buf[], size_t size);
void *syscall_exec(const char* name, char *const argv[]);
void *syscall_fork(void);
void *syscall_exit(void);
void *syscall_mbox_call(unsigned char ch, unsigned int *mbox);
void *syscall_kill_pid(int pid);
void *syscall_signal(int signal, void (*handler)(void));
void *syscall_kill(int pid, int signal);
void *syscall_show_info(void);

syscall_funcp syscall_table[] = {
    (syscall_funcp) syscall_getpid,     // 0
    (syscall_funcp) syscall_uartread,
    (syscall_funcp) syscall_uartwrite,
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

    if (syscall_num >= ARRAY_SIZE(syscall_table)) {
        // Invalid syscall
        return;
    }

    enable_interrupt();
    (syscall_table[syscall_num])(
        regs.x0,
        regs.x1,
        regs.x2,
        regs.x3,
        regs.x4,
        regs.x5
    );
    disable_interrupt();
}

void *syscall_getpid(void)
{
    // TODO
    return NULL;
}

void *syscall_uartread(char buf[], size_t size)
{
    // TODO
    return NULL;
}

void *syscall_uartwrite(const char buf[], size_t size)
{
    // TODO
    return NULL;
}

void *syscall_exec(const char* name, char *const argv[])
{
    // TODO
    return NULL;
}

void *syscall_fork(void)
{
    // TODO
    return NULL;
}

void *syscall_exit(void)
{
    exit_user_prog();

    return NULL;
}

void *syscall_mbox_call(unsigned char ch, unsigned int *mbox)
{
    // TODO
    return NULL;
}

void *syscall_kill_pid(int pid)
{
    // TODO
    return NULL;
}

void *syscall_signal(int signal, void (*handler)(void))
{
    // TODO
    return NULL;
}

void *syscall_kill(int pid, int signal)
{
    // TODO
    return NULL;
}

// Print the content of spsr_el1, elr_el1 and esr_el1
void *syscall_show_info(void)
{
    uint64 spsr_el1;
    uint64 elr_el1;
    uint64 esr_el1;

    spsr_el1 = read_sysreg(spsr_el1);
    elr_el1 = read_sysreg(elr_el1);
    esr_el1 = read_sysreg(esr_el1);

    uart_printf("[TEST] spsr_el1: %llx; elr_el1: %llx; esr_el1: %llx\r\n",
        spsr_el1, elr_el1, esr_el1);

    return NULL;
}