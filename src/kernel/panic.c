#include <panic.h>
#include <mini_uart.h>

void panic(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    uart_sync_printf("\r\n");
    
    uart_sync_vprintf(fmt, args);
    va_end(args);

    uart_sync_printf("\r\n");

    // TODO: Show more information

    // Never return
    while (1) {}
}