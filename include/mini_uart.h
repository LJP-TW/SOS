#ifndef	_MINI_UART_H
#define	_MINI_UART_H

int uart_recvline(char *buff, int maxlen);
void uart_init(void);
char uart_recv(void);
void uart_send(char c);
void uart_send_string(char *str);
void uart_send_uint(unsigned int num);

#endif  /* _MINI_UART_H */