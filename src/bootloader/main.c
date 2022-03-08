#include <mini_uart.h>
#include <string.h>
#include <BCM2837.h>
#include <rpi3.h>

#define BUFSIZE 0x100

extern char _kernel[];
char shell_buf[BUFSIZE];

typedef void (*funcp)(void);

static void load_kernel(void)
{
    // Loading kernel Protocol:
    //  4 bytes: The kernel image length n
    //  n bytes: The kernel image
    // Bootloader will store kernel to 0x80000 and then jump to it
    unsigned int len;
    char *p = _kernel;

    uart_send_string("\r\n");
    uart_send_string("[*] Kernel base address: ");
    uart_send_uint((unsigned int)_kernel);

    len = uart_recv_uint();

    uart_send_string("\r\n");
    uart_send_string("[*] Kernel image length: ");
    uart_send_uint(len);

    while (len--) {
        *p++ = uart_recv();
    }

    uart_send_string("\r\n");
    uart_send_string("[*] Kernel loaded!");
    uart_send_string("\r\n");

    // Execute kernel
    ((funcp)_kernel)();
}

void start_bootloader(void)
{
    uart_init();
    uart_send_string("[*] Bootloader\r\n");

    while (1) {
        uart_send_string("# ");
        uart_recvline(shell_buf, BUFSIZE);
        uart_send_string("\r\n");
        if (!strcmp("help", shell_buf)) {
            uart_send_string(
                "help\t: "   "print this help menu" "\r\n"
                "hello\t: "  "print Hello World!"   "\r\n"
                "hwinfo\t: " "print hardware info"  "\r\n"
                "load\t: "   "load kernel"          "\r\n"
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
        } else if (!strcmp("load", shell_buf)) {
            uart_send_string("[*] Loading kernel ...");
            load_kernel();
        } else {
            uart_send_string(shell_buf);
        }
        uart_send_string("\r\n");
    }
}