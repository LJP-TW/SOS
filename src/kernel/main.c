#include <mini_uart.h>
#include <string.h>
#include <BCM2837.h>
#include <rpi3.h>
#include <cpio.h>
#include <fdt.h>

#define BUFSIZE 0x100

uint64 _initramfs;
static char shell_buf[BUFSIZE];
char *fdt_base;

static void cmd_help(void)
{
    uart_printf(
                "cat <filename>\t: " "get file content"  "\r\n"
                "help\t: "   "print this help menu" "\r\n"
                "hello\t: "  "print Hello World!"   "\r\n"
                "hwinfo\t: " "print hardware info"  "\r\n"
                "ls\t: " "list files in initramfs"  "\r\n"
                "parsedtb\t: " "parse devicetree blob (dtb)"  "\r\n"
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

static void cmd_reboot(void)
{
    uart_printf("Reboot!\r\n");
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

static void cmd_parsedtb(void)
{
    fdt_traversal(fdt_base);
}

static int shell_read_cmd(void)
{
    return uart_recvline(shell_buf, BUFSIZE);
}

static void shell(void)
{
    while (1) {
        int cmd_len;
        uart_printf("# ");
        cmd_len = shell_read_cmd();
        uart_printf("\r\n");
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
        } else if (!strcmp("parsedtb", shell_buf)) {
            cmd_parsedtb();
        } else if (!strncmp("cat", shell_buf, 3)) {
            if (cmd_len >= 5) {
                cmd_cat(&shell_buf[4]);
            }
        } else {
            // Just echo back
            uart_printf("%s\r\n", shell_buf);
        }
    }
}

static int initramfs_fdt_parser(int level, char *cur, char *dt_strings)
{
    struct fdt_node_header *nodehdr = cur;
    struct fdt_property *prop;

    uint32 tag = fdtn_tag(nodehdr);

    switch (tag) {
    case FDT_PROP:
        prop = nodehdr;
        if (!strcmp("linux,initrd-start", dt_strings + fdtp_nameoff(prop))) {
            _initramfs = fdt32_ld(&prop->data);
            uart_printf("[*] initrd addr: %x\r\n", _initramfs);
            return 1;
        }
        break;
    case FDT_BEGIN_NODE:
    case FDT_END_NODE:
    case FDT_NOP:
    case FDT_END:
        break;
    }

    return 0;
}

static void initramfs_init()
{
    // Get initramfs address from devicetree
    _initramfs = 0;
    parse_dtb(fdt_base, initramfs_fdt_parser);
    if (!_initramfs) {
        uart_printf("[x] Cannot find initrd address!!!\r\n");
    }
}

void start_kernel(char *fdt)
{
    fdt_base = fdt;

    uart_init();
    uart_printf("[*] fdt base: %x\r\n", fdt_base);
    uart_printf("[*] Kernel\r\n");

    initramfs_init();

    shell();
}