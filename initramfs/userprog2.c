#include <stddef.h>
#include <stdarg.h>

#define SIGN        1

typedef unsigned long long int uint64;
typedef unsigned int uint32;
typedef unsigned short uint16;
typedef unsigned char uint8;

typedef long long int int64;
typedef int int32;
typedef short int16;
typedef char int8;

#define SYS_GETPID      0
#define SYS_UART_RECV   1
#define SYS_UART_WRITE  2
#define SYS_EXEC        3
#define SYS_EXIT        5
#define SYS_MBOX_CALL   6
#define SYS_KILL_PID    7

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

void uart_recv(const char buf[], size_t size)
{
    syscall(SYS_UART_RECV, (void *)buf, (void *)size, 0, 0, 0, 0);
}

void uart_write(const char buf[], size_t size)
{
    syscall(SYS_UART_WRITE, (void *)buf, (void *)size, 0, 0, 0, 0);
}

static void uart_send_char(char c)
{
    uart_write(&c, 1);
}

static void uart_send_string(const char *str)
{
    for (int i = 0; str[i] != '\0'; i++) {
        uart_send_char(str[i]);
    }
}

static void uart_send_num(int64 num, int base, int type)
{
    static const char digits[16] = "0123456789ABCDEF";
    char tmp[66];
    int i;

    if (type | SIGN) {
        if (num < 0) {
            uart_send_char('-');
        }
    }

    i = 0;

    if (num == 0) {
        tmp[i++] = '0';
    } else {
        while (num != 0) {
            uint8 r = (uint32)num % base;
            num = (uint32)num / base;
            tmp[i++] = digits[r];
        }
    }

    while (--i >= 0) {
        uart_send_char(tmp[i]);
    }
}

static void _uart_printf(const char *fmt, va_list args)
{
    const char *s;
    char c;
    uint64 num;
    char width;

    for (; *fmt; ++fmt) {
        if (*fmt != '%') {
            uart_send_char(*fmt);
            continue;
        }

        ++fmt;

        // Get width
        width = 0;
        if (fmt[0] == 'l' && fmt[1] == 'l') {
            width = 1;
            fmt += 2;
        }

        switch (*fmt) {
        case 'c':
            c = va_arg(args, uint32) & 0xff;
            uart_send_char(c);
            continue;
        case 'd':
            if (width) {
                num = va_arg(args, int64);
            } else {
                num = va_arg(args, int32);
            }
            uart_send_num(num, 10, SIGN);
            continue;
        case 's':
            s = va_arg(args, char *);
            uart_send_string(s);
            continue;
        case 'x':
            if (width) {
                num = va_arg(args, uint64);
            } else {
                num = va_arg(args, uint32);
            }
            uart_send_num(num, 16, 0);
            continue;
        }
    }
}

void uart_printf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    _uart_printf(fmt, args);

    va_end(args);
}

void exec(const char *name, char *const argv[])
{
    syscall(SYS_EXEC, (void *)name, (void *)argv, 0, 0, 0, 0);
}

void exit(void)
{
    syscall(SYS_EXIT, 0, 0, 0, 0, 0, 0);
}

void mbox_call(unsigned char ch, unsigned int *mbox)
{
    syscall(SYS_MBOX_CALL, (void *)(uint64)ch, mbox, 0, 0, 0, 0);
}

void kill_pid(int pid)
{
    syscall(SYS_KILL_PID, (void *)(uint64)pid, 0, 0, 0, 0, 0);
}

/* Channels */
#define MAILBOX_CH_PROP    8

/* Tags */
#define TAG_REQUEST_CODE    0x00000000
#define END_TAG             0x00000000

/* Tag identifier */
#define GET_BOARD_REVISION  0x00010002

/* Others */
#define REQUEST_CODE        0x00000000

static unsigned int __attribute__((aligned(0x10))) mailbox[16];

// It should be 0xa020d3 for rpi3 b+
unsigned int get_board_revision(void)
{
    mailbox[0] = 7 * 4;              // Buffer size in bytes
    mailbox[1] = REQUEST_CODE;
    // Tags begin
    mailbox[2] = GET_BOARD_REVISION; // Tag identifier
    mailbox[3] = 4;                  // Value buffer size in bytes
    mailbox[4] = TAG_REQUEST_CODE;   // Request/response codes
    mailbox[5] = 0;                  // Optional value buffer
    // Tags end
    mailbox[6] = END_TAG;

    mbox_call(MAILBOX_CH_PROP, mailbox); // Message passing procedure call

    return mailbox[5];
}

int start(void)
{
    char buf1[0x10] = { 0 };
    char buf2[0x10] = { 0 };
    int pid, idx, revision;
    
    kill_pid(2);

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
    
    uart_printf("[User] buf1: %s\r\n", buf1);

    revision = get_board_revision();

    uart_printf("revision: %x\r\n", revision);

    exec("userprog1", NULL);

    return 0;
}