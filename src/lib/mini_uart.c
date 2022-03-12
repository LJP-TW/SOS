// Ref: https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson01/src/mini_uart.c
#include <stdarg.h>
#include <mini_uart.h>
#include <BCM2837.h>
#include <utils.h>

#define SIGN        1

uint32 uart_recv_uint(void)
{
    char buf[4];
    
    for (int i = 0; i < 4; ++i) {
        buf[i] = uart_recv();
    }

    return *((uint32*)buf);
}

uint32 uart_recvline(char *buff, int maxlen)
{
    uint32 cnt = 0;

    // Reserve space for NULL byte
    maxlen--;

    while (maxlen) {
        char c = uart_recv();

        if (c == '\r') {
            // TODO: what if c == '\0'?
            break;
        }

        uart_send(c);
        *buff = c;
        buff++;
        cnt += 1;
        maxlen -= 1;
    }

    *buff = 0;

    return cnt;
}

void uart_send(char c)
{
    while (!(get32(AUX_MU_LSR_REG) & 0x20)) {};

    put32(AUX_MU_IO_REG, c);
}

void uart_sendn(char *str, int n)
{
    while (n--) {
        uart_send(*str++);
    }
}

char uart_recv(void)
{
    while (!(get32(AUX_MU_LSR_REG) & 0x01)) {};

    return (get32(AUX_MU_IO_REG) & 0xFF);
}

void uart_send_string(const char *str)
{
    for (int i = 0; str[i] != '\0'; i++) {
        uart_send(str[i]);
    }
}

/*
 * Send hex uint to UART
 */
void uart_send_uint(unsigned int num)
{
    int shift = 32;
    uart_send_string("0x");

    while (shift) {
        int n = num >> (shift - 4) & 0xf;

        if (0 <= n && n <= 9) {
            char c = '0' + n;
            uart_send(c);
        } else {
            char c = 'A' + n - 10;
            uart_send(c);
        }

        shift -= 4;
    }
}

// Ref: https://elixir.bootlin.com/linux/v3.5/source/arch/x86/boot/printf.c#L43
/*
 * @num: output number 
 * @base: 10 or 16
 */
static void uart_send_num(int32 num, int base, int type)
{
    static const char digits[16] = "0123456789ABCDEF";
    char tmp[66];
    int i;

    if (type | SIGN) {
        if (num < 0) {
            uart_send('-');
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
        uart_send(tmp[i]);
    }
}

// Ref: https://elixir.bootlin.com/linux/v3.5/source/arch/x86/boot/printf.c#L115
void uart_printf(char *fmt, ...)
{
    // uart_send_string(fmt);
    // uart_send_string("\r\n");

    const char *s;
    char c;
    uint32 num;   

    va_list args;
    va_start(args, fmt);

    for (; *fmt; ++fmt) {
        if (*fmt != '%') {
            uart_send(*fmt);
            continue;
        }

        ++fmt;
        switch (*fmt) {
        case 'c':
            c = va_arg(args, uint32) & 0xff;
            uart_send(c);
            continue;
        case 'd':
            num = va_arg(args, int32);
            uart_send_num(num, 10, SIGN);
            continue;
        case 's':
            s = va_arg(args, char *);
            uart_send_string(s);
            continue;
        case 'x':
            num = va_arg(args, uint32);
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
}