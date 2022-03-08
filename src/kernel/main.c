#include <mini_uart.h>
#include <string.h>
#include <BCM2837.h>
#include <rpi3.h>
#include <cpio.h>

#define BUFSIZE 0x100

extern char _initramfs[];
static char shell_buf[BUFSIZE];

static void cmd_help(void)
{
    uart_send_string(
                "cat <filename>\t: " "get file content"  "\r\n"
                "help\t: "   "print this help menu" "\r\n"
                "hello\t: "  "print Hello World!"   "\r\n"
                "hwinfo\t: " "print hardware info"  "\r\n"
                "ls\t: " "list files in initramfs"  "\r\n"
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

static void cmd_reboot(void)
{
    uart_send_string("Reboot!\r\n");
    BCM2837_reset(10);
}

static void cmd_ls(void)
{
    cpio_ls(_initramfs);
}

static void cmd_cat(char *filename)
{
    cpio_cat(_initramfs, filename);
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
        } else if (!strcmp("reboot", shell_buf)) {
            cmd_reboot();
        } else if (!strcmp("ls", shell_buf)) {
            cmd_ls();
        } else if (!strncmp("cat", shell_buf, 3)) {
            if (cmd_len >= 5) {
                cmd_cat(&shell_buf[4]);
            }
        } else {
            // Just echo back
            uart_send_string(shell_buf);
            uart_send_string("\r\n");
        }
    }
}

void start_kernel(void)
{
    uart_init();
    uart_send_string("[*] Kernel\r\n");

    shell();
}