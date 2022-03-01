#include <mini_uart.h>
#include <string.h>
#include <BCM2837.h>
#include <rpi3.h>

#define BUFSIZE 0x100

char shell_buf[BUFSIZE];

void start_kernel(void)
{
    uart_init();
    uart_send_string("Hello, world!\r\n");

    while (1) {
        uart_send_string("# ");
        uart_recvline(shell_buf, BUFSIZE);
        uart_send_string("\r\n");
        if (!strcmp("help", shell_buf)) {
            uart_send_string(
                "help\t: "   "print this help menu" "\r\n"
                "hello\t: "  "print Hello World!"   "\r\n"
                "hwinfo\t: " "print hardware info"   "\r\n"
                "reboot\t: " "reboot the device"
            );
        } else if (!strcmp("hello", shell_buf)) {
            uart_send_string("Hello World!");
        } else if (!strcmp("hwinfo", shell_buf)) {
            unsigned int rev;
            struct arm_memory_info ami;

            // Board revision
            rev = get_board_revision();
            uart_send_string("[*] Revision: ");
            uart_send_uint(rev);
            uart_send_string("\r\n");

            // ARM memory base address and size
            get_arm_memory(&ami);
            uart_send_string("[*] ARM memory base address: ");
            uart_send_uint(ami.baseaddr);
            uart_send_string("\r\n");
            uart_send_string("[*] ARM memory size: ");
            uart_send_uint(ami.size);
        } else if (!strcmp("reboot", shell_buf)) {
            uart_send_string("Reboot!");
            BCM2837_reset(10);
        } else {
            uart_send_string(shell_buf);
        }
        uart_send_string("\r\n");
    }
}