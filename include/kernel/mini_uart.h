#ifndef _MINI_UART_H
#define _MINI_UART_H

#include <types.h>

uint32 uart_recvline(char *buff, int maxlen);
void uart_init(void);
char uart_recv(void);
void uart_recvn(char *buff, int n);
uint32 uart_recv_uint(void);
void uart_send(char c);
void uart_sendn(const char *str, int n);
void uart_printf(const char *fmt, ...);

void uart_sync_printf(const char *fmt, ...);

int uart_irq_check(void);

/* Switch asynchronous/synchronous mode for uart RW */
int uart_switch_mode(void);

#endif  /* _MINI_UART_H */