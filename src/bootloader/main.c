#include <mini_uart.h>
#include <string.h>
#include <BCM2837.h>
#include <rpi3.h>

#define BUFSIZE 0x100

extern char _kernel[];
char shell_buf[BUFSIZE];
char *fdt_base;

typedef void (*kernel_funcp)(char *fdt);

static void load_kernel(void)
{
    // Loading kernel Protocol:
    //  4 bytes: The kernel image length n
    //  n bytes: The kernel image
    // Bootloader will store kernel to 0x80000 and then jump to it
    unsigned int len;
    char *p = _kernel;

    uart_printf("[*] Kernel base address: %x\r\n", _kernel);

    len = uart_recv_uint();

    uart_printf("[*] Kernel image length: %d\r\n", len);

    while (len--) {
        *p++ = uart_recv();
    }

    // Execute kernel
    ((kernel_funcp)_kernel)(fdt_base);
}

static void cmd_help(void)
{
    uart_printf(
                "help\t: "   "print this help menu" "\r\n"
                "hello\t: "  "print Hello World!"   "\r\n"
                "hwinfo\t: " "print hardware info"  "\r\n"
                "load\t: "   "load kernel"          "\r\n"
                "reboot\t: " "reboot the device"    "\r\n"
            );
}

static void cmd_hello(void)
{
    uart_printf("Hello World!\r\n");
}

static void cmd_hwinfo(void)
{
    unsigned int rev;
    struct arm_memory_info ami;

    // Board revision
    rev = get_board_revision();
    uart_printf("[*] Revision: %x\r\n", rev);

    // ARM memory base address and size
    get_arm_memory(&ami);
    uart_printf("[*] ARM memory base address: %x\r\n", ami.baseaddr);
    uart_printf("[*] ARM memory size: %d\r\n", ami.size);
}

static void cmd_loadkernel(void)
{
    uart_printf("[*] Loading kernel ...\r\n");
    load_kernel();
}

static void cmd_reboot(void)
{
    BCM2837_reset(10);
}

static int shell_read_cmd(void)
{
    return uart_recvline(shell_buf, BUFSIZE);
}

static void shell(void)
{
    // One char maybe be received 
    uart_recv();

    while (1) {
        uart_printf("# ");
        shell_read_cmd();
        uart_printf("\r\n");
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
            uart_printf("%s\r\n", shell_buf);
        }        
    }
}

void start_bootloader(char *fdt)
{
    fdt_base = fdt;

    uart_init();
    uart_printf("[*] Bootloader\r\n");

    shell();
}