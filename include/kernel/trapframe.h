#ifndef _TRAPFRAME_H
#define _TRAPFRAME_H

#include <types.h>
#include <mini_uart.h>

typedef struct {
    uint64 x0;
    uint64 x1;
    uint64 x2;
    uint64 x3;
    uint64 x4;
    uint64 x5;
    uint64 x6;
    uint64 x7;
    uint64 x8;
    uint64 x9;
    uint64 x10;
    uint64 x11;
    uint64 x12;
    uint64 x13;
    uint64 x14;
    uint64 x15;
    uint64 x16;
    uint64 x17;
    uint64 x18;
    uint64 x19;
    uint64 x20;
    uint64 x21;
    uint64 x22;
    uint64 x23;
    uint64 x24;
    uint64 x25;
    uint64 x26;
    uint64 x27;
    uint64 x28;
    uint64 x29;
    uint64 x30;
    void *sp_el0;
    void *elr_el1;
    void *spsr_el1;
} trapframe;

static inline void show_trapframe(trapframe *regs)
{
    uart_sync_printf("\r\n[*] Trapframe:\r\n");
    uart_sync_printf("\tx0: %llx\r\n", regs->x0);
    uart_sync_printf("\tx1: %llx\r\n", regs->x1);
    uart_sync_printf("\tx2: %llx\r\n", regs->x2);
    uart_sync_printf("\tx3: %llx\r\n", regs->x3);
    uart_sync_printf("\tx4: %llx\r\n", regs->x4);
    uart_sync_printf("\tx5: %llx\r\n", regs->x5);
    uart_sync_printf("\tx6: %llx\r\n", regs->x6);
    uart_sync_printf("\tx7: %llx\r\n", regs->x7);
    uart_sync_printf("\tx8: %llx\r\n", regs->x8);
    uart_sync_printf("\tx9: %llx\r\n", regs->x9);
    uart_sync_printf("\tx10: %llx\r\n", regs->x10);
    uart_sync_printf("\tx11: %llx\r\n", regs->x11);
    uart_sync_printf("\tx12: %llx\r\n", regs->x12);
    uart_sync_printf("\tx13: %llx\r\n", regs->x13);
    uart_sync_printf("\tx14: %llx\r\n", regs->x14);
    uart_sync_printf("\tx15: %llx\r\n", regs->x15);
    uart_sync_printf("\tx16: %llx\r\n", regs->x16);
    uart_sync_printf("\tx17: %llx\r\n", regs->x17);
    uart_sync_printf("\tx18: %llx\r\n", regs->x18);
    uart_sync_printf("\tx19: %llx\r\n", regs->x19);
    uart_sync_printf("\tx20: %llx\r\n", regs->x20);
    uart_sync_printf("\tx21: %llx\r\n", regs->x21);
    uart_sync_printf("\tx22: %llx\r\n", regs->x22);
    uart_sync_printf("\tx23: %llx\r\n", regs->x23);
    uart_sync_printf("\tx24: %llx\r\n", regs->x24);
    uart_sync_printf("\tx25: %llx\r\n", regs->x25);
    uart_sync_printf("\tx26: %llx\r\n", regs->x26);
    uart_sync_printf("\tx27: %llx\r\n", regs->x27);
    uart_sync_printf("\tx28: %llx\r\n", regs->x28);
    uart_sync_printf("\tx29: %llx\r\n", regs->x29);
    uart_sync_printf("\tx30: %llx\r\n", regs->x30);
    uart_sync_printf("\tsp_el0  : %llx\r\n", regs->sp_el0);
    uart_sync_printf("\telr_el1 : %llx\r\n", regs->elr_el1);
    uart_sync_printf("\tspsr_el1: %llx\r\n", regs->spsr_el1);
}

#endif /* _TRAPFRAME_H */