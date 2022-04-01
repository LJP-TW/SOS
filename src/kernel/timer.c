#include <timer.h>
#include <utils.h>
#include <mini_uart.h>

#define CORE0_IRQ_SOURCE 0x40000060

int timer_show_enable;
uint64 timer_boot_cnt;

static void timer_set_boot_cnt()
{
    timer_boot_cnt = read_sysreg(cntpct_el0);
}

void timer_init()
{
    timer_set_boot_cnt();
}

void timer_irq_handler()
{
    uint32 core0_irq_src = get32(CORE0_IRQ_SOURCE);

    if (core0_irq_src & 0x02) {
        // Set next timer before calling any functions which may interrupt
        uint32 cntfrq_el0 = read_sysreg(cntfrq_el0);
        write_sysreg(cntp_tval_el0, cntfrq_el0 * 2);

        if (timer_show_enable) {
            uint64 cntpct_el0 = read_sysreg(cntpct_el0);
            uart_printf("[Boot time: %lld seconds...]\r\n", 
                        (cntpct_el0 - timer_boot_cnt) / cntfrq_el0);
        }
    }
}

void timer_switch_info()
{
    timer_show_enable = !timer_show_enable;
}