#include <stddef.h>

typedef unsigned long long int uint64;

#define SYS_GETPID      0
#define SYS_UART_RECV   1
#define SYS_UART_WRITE  2
#define SYS_EXIT        5

int start(void) __attribute__((section(".start")));

uint64 syscall(uint64 syscall_num,
               void *x0,
               void *x1,
               void *x2,
               void *x3,
               void *x4,
               void *x5)
{
    uint64 result;

    asm volatile (
        "ldr x8, %0\n"
        "ldr x0, %1\n"
        "ldr x1, %2\n"
        "ldr x2, %3\n"
        "ldr x3, %4\n"
        "ldr x4, %5\n"
        "ldr x5, %6\n"
        "svc 0\n"
        :: "m" (syscall_num), "m" (x0), "m" (x1), 
           "m" (x2), "m" (x3), "m" (x4), "m" (x5)
    );

    asm volatile (
        "str x0, %0\n"
        : "=m" (result)
    );

    return result;
}

int getpid()
{
    int pid = syscall(SYS_GETPID, 0, 0, 0, 0, 0, 0);
    return pid;
}

void exit(void)
{
    syscall(SYS_EXIT, 0, 0, 0, 0, 0, 0);
}

void uart_recv(const char buf[], size_t size)
{
    syscall(SYS_UART_RECV, (void *)buf, (void *)size, 0, 0, 0, 0);
}

void uart_write(const char buf[], size_t size)
{
    syscall(SYS_UART_WRITE, (void *)buf, (void *)size, 0, 0, 0, 0);
}

int start(void)
{
    char buf1[0x10] = { 0 };
    char buf2[0x10] = { 0 };
    int pid, idx;
    
    idx = 0;
    pid = getpid();

    while (pid) {
        buf1[idx++] = '0' + pid % 10;
        pid /= 10;
    }

    for (int i = 0; i < idx; ++i) {
        buf2[i] = buf1[idx - i - 1];
    }

    buf2[idx] = '\r';
    buf2[idx+1] = '\n';

    uart_write(buf2, idx+2);

    uart_recv(buf1, 8);
    
    uart_write("[User] buf1: ", 13);
    uart_write(buf1, 8);
    uart_write("\r\n", 2);

    exit();

    return 0;
}