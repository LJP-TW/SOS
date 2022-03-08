// Ref: https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson01/src/mini_uart.c
#include <mini_uart.h>
#include <BCM2837.h>
#include <utils.h>

unsigned int uart_recv_uint(void)
{
    char buf[4];
    
    for (int i = 0; i < 4; ++i) {
        buf[i] = uart_recv();
    }

    return *((unsigned int*)buf);
}

int uart_recvline(char *buff, int maxlen)
{
    int cnt = 0;

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

void uart_send_string(char *str)
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