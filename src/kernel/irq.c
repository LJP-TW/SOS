#include <irq.h>
#include <mini_uart.h>
#include <utils.h>
#include <timer.h>
#include <BCM2837.h>

void irq_handler()
{
    timer_irq_handler();
    uart_irq_handler();
}

void exception_default_handler(uint32 n)
{
    uart_printf("[exception] %d\r\n", n);
}

void irq1_enable(int bit)
{
    put32(IRQ_ENABLE_1_REG, 1 << bit);
}