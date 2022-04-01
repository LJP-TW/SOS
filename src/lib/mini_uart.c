// Ref: https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson01/src/mini_uart.c
#include <stdarg.h>
#include <mini_uart.h>
#include <BCM2837.h>
#include <utils.h>

#define SIGN        1

#define BUFSIZE 0x100

// UART asynchronous/synchronous mode
// 0: Synchronous mode
// 1: Asynchronous mode
static int uart_sync_mode;

// UART read / write function pointer
typedef char (*recvfp)(void);
typedef void (*sendfp)(char);

recvfp uart_recv_fp;
sendfp uart_send_fp;

// Data structure for async RW
static char r_ringbuf[BUFSIZE];
static char w_ringbuf[BUFSIZE];
static int r_head, r_tail;
static int w_head, w_tail;

static char uart_asyn_recv(void)
{
    while (r_head == r_tail) {}

    char tmp = r_ringbuf[r_head];
    r_head = (r_head + 1) % BUFSIZE;

    return tmp;
}

static void uart_asyn_send(char c)
{
    while (w_head == (w_tail + 1) % BUFSIZE) {
    }

    w_ringbuf[w_tail] = c;
    w_tail = (w_tail + 1) % BUFSIZE;

    // Enable transmit interrupt
    uint32 ier = get32(AUX_MU_IER_REG);
    ier = ier | 0x02;
    put32(AUX_MU_IER_REG, ier);

    enable_interrupt();
}

static char uart_sync_recv(void)
{
    while (!(get32(AUX_MU_LSR_REG) & 0x01)) {};

    return (get32(AUX_MU_IO_REG) & 0xFF);
}

static void uart_sync_send(char c)
{
    while (!(get32(AUX_MU_LSR_REG) & 0x20)) {};

    put32(AUX_MU_IO_REG, c);
}

char uart_recv(void)
{
    return (uart_recv_fp)();
}

void uart_send(char c)
{
    (uart_send_fp)(c);
}

uint32 uart_recv_uint(void)
{
    char buf[4];
    
    for (int i = 0; i < 4; ++i) {
        buf[i] = (uart_recv_fp)();
    }

    return *((uint32*)buf);
}

uint32 uart_recvline(char *buff, int maxlen)
{
    uint32 cnt = 0;

    // Reserve space for NULL byte
    maxlen--;

    while (maxlen) {
        char c = (uart_recv_fp)();

        if (c == '\r') {
            // TODO: what if c == '\0'?
            break;
        }

        (uart_send_fp)(c);
        *buff = c;
        buff++;
        cnt += 1;
        maxlen -= 1;
    }

    *buff = 0;

    return cnt;
}

void uart_sendn(char *str, int n)
{
    while (n--) {
        (uart_send_fp)(*str++);
    }
}

static void uart_send_string(const char *str)
{
    for (int i = 0; str[i] != '\0'; i++) {
        (uart_send_fp)(str[i]);
    }
}

// Ref: https://elixir.bootlin.com/linux/v3.5/source/arch/x86/boot/printf.c#L43
/*
 * @num: output number 
 * @base: 10 or 16
 */
static void uart_send_num(int64 num, int base, int type)
{
    static const char digits[16] = "0123456789ABCDEF";
    char tmp[66];
    int i;

    if (type | SIGN) {
        if (num < 0) {
            (uart_send_fp)('-');
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
        (uart_send_fp)(tmp[i]);
    }
}

// Ref: https://elixir.bootlin.com/linux/v3.5/source/arch/x86/boot/printf.c#L115
void uart_printf(char *fmt, ...)
{
    const char *s;
    char c;
    uint64 num;
    char width;

    va_list args;
    va_start(args, fmt);

    for (; *fmt; ++fmt) {
        if (*fmt != '%') {
            (uart_send_fp)(*fmt);
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
            (uart_send_fp)(c);
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

void uart_init(void)
{
    unsigned int selector;

    selector = get32(GPFSEL1);
    selector &= ~(7 << 12);            // Clean gpio14
    selector |= 2 << 12;               // Set alt5 for gpio14
    selector &= ~(7 << 15);            // Clean gpio15
    selector |= 2 << 15;               // Set alt5 for gpio15
    put32(GPFSEL1, selector);

    put32(GPPUD, 0);
    delay(150);
    put32(GPPUDCLK0, (1 << 14) | (1 << 15));
    delay(150);
    put32(GPPUDCLK0, 0);

    put32(AUX_ENABLES, 1);             // Enable mini uart (this also enables access to its registers)
    put32(AUX_MU_CNTL_REG, 0);         // Disable auto flow control and disable receiver and transmitter (for now)
    put32(AUX_MU_IER_REG, 0);          // Disable receive and transmit interrupts
    put32(AUX_MU_LCR_REG, 3);          // Enable 8 bit mode
    put32(AUX_MU_MCR_REG, 0);          // Set RTS line to be always high
    put32(AUX_MU_BAUD_REG, 270);       // Set baud rate to 115200
    put32(AUX_MU_IIR_REG, 6);          // Clear the Rx/Tx FIFO
    put32(AUX_MU_CNTL_REG, 3);         // Finally, enable transmitter and receiver

    // UART start from synchronous mode
    uart_sync_mode = 0;
    uart_recv_fp = uart_sync_recv;
    uart_send_fp = uart_sync_send;
}

void uart_irq_handler(void)
{
    uint32 iir = get32(AUX_MU_IIR_REG);

    if (iir & 0x01) {
        // No interrupt
        return;
    }

    if (iir & 0x02) {
        // Transmit holding register empty
        if (w_head == w_tail) {
            // Transmit complete
            // Disable interrupt
            uint32 ier = get32(AUX_MU_IER_REG);

            ier = ier & ~(0x02);

            put32(AUX_MU_IER_REG, ier);
        } else {
            put32(AUX_MU_IO_REG, w_ringbuf[w_head]);
            w_head = (w_head + 1) % BUFSIZE;
        }
    } else if (iir & 0x04) {
        // Receiver holds valid byte
        if (r_head != (r_tail + 1) % BUFSIZE) {
            r_ringbuf[r_tail] = get32(AUX_MU_IO_REG) & 0xFF;
            r_tail = (r_tail + 1) % BUFSIZE;
        }
    }
}

int uart_switch_mode(void)
{
    uart_sync_mode = !uart_sync_mode;
    uint32 ier = get32(AUX_MU_IER_REG);
    ier = ier & ~(0x03);

    if (uart_sync_mode == 0) {
        // Synchronous mode
        uart_recv_fp = uart_sync_recv;
        uart_send_fp = uart_sync_send;

        // Disable interrupt
        put32(AUX_MU_IER_REG, ier);
    } else {
        // Asynchronous mode
        uart_recv_fp = uart_asyn_recv;
        uart_send_fp = uart_asyn_send;

        // Clear the Rx/Tx FIFO
        put32(AUX_MU_IIR_REG, 6);

        // Enable receive interrupt
        ier = ier | 0x01;
        put32(AUX_MU_IER_REG, ier);
    }

    return uart_sync_mode;
}