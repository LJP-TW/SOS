#include <irq.h>
#include <mini_uart.h>
#include <utils.h>
#include <timer.h>

void irq_handler()
{
    timer_irq_handler();
}

void exception_default_handler(uint32 n)
{
    uart_printf("[exception] %d\r\n", n);
}