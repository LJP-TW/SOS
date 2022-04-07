#ifndef _MINI_UART_H
#define _MINI_UART_H

#include <types.h>

uint32 uart_recvline(char *buff, int maxlen);
void uart_init(void);
char uart_recv(void);
uint32 uart_recv_uint(void);
void uart_send(char c);
void uart_sendn(char *str, int n);
void uart_printf(char *fmt, ...);

#endif  /* _MINI_UART_H */