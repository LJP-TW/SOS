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

    uart_send_string("[*] Kernel base address: ");
    uart_send_uint((unsigned int)_kernel);
    uart_send_string("\r\n");

    len = uart_recv_uint();

    uart_send_string("[*] Kernel image length: ");
    uart_send_uint(len);
    uart_send_string("\r\n");

    while (len--) {
        *p++ = uart_recv();
    }

    uart_send_string("[*] Kernel loaded!\r\n");

    // Execute kernel
    ((funcp)_kernel)();
}

static void cmd_help(void)
{
    uart_send_string(
                "help\t: "   "print this help menu" "\r\n"
                "hello\t: "  "print Hello World!"   "\r\n"
                "hwinfo\t: " "print hardware info"  "\r\n"
                "load\t: "   "load kernel"          "\r\n"
                "reboot\t: " "reboot the device"    "\r\n"
            );
}

static void cmd_hello(void)
{
    uart_send_string("Hello World!\r\n");
}

static void cmd_hwinfo(void)
{
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
    uart_send_string("\r\n");
}

static void cmd_loadkernel(void)
{
    uart_send_string("[*] Loading kernel ...\r\n");
    load_kernel();
}

static void cmd_reboot(void)
{
    uart_send_string("Reboot!\r\n");
    BCM2837_reset(10);
}

static int shell_read_cmd(void)
{
    return uart_recvline(shell_buf, BUFSIZE);
}

static void shell(void)
{
    while (1) {
        int cmd_len;
        uart_send_string("# ");
        cmd_len = shell_read_cmd();
        uart_send_string("\r\n");
        if (!strcmp("help", shell_buf)) {
            cmd_help();
        } else if (!strcmp("hello", shell_buf)) {
            cmd_hello();
        } else if (!strcmp("hwinfo", shell_buf)) {
            cmd_hwinfo();
        } else if (!strcmp("load", shell_buf)) {
            cmd_loadkernel();
        } else if (!strcmp("reboot", shell_buf)) {
            cmd_reboot();
        } else {
            // Just echo back
            uart_send_string(shell_buf);
            uart_send_string("\r\n");
        }        
    }
}

void start_bootloader(void)
{
    uart_init();
    uart_send_string("[*] Bootloader\r\n");

    shell();
}