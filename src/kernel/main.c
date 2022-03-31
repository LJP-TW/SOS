#include <mini_uart.h>
#include <string.h>
#include <BCM2837.h>
#include <rpi3.h>
#include <cpio.h>
#include <fdt.h>
#include <mem.h>
#include <exec.h>
#include <utils.h>
#include <timer.h>

#define BUFSIZE 0x100

char *_initramfs;
static char shell_buf[BUFSIZE];
char *fdt_base;

static void cmd_alloc(char *ssize)
{
    int size = atoi(ssize);
    
    char *p = simple_malloc(size);

    uart_printf("[*] p: %x\r\n", p);
}

static void cmd_help(void)
{
    uart_printf(
                "alloc <size>\t: "   "test simple allocator" "\r\n"
                "cat <filename>\t: " "get file content"  "\r\n"
                "exec <filename>\t: " "execute file"  "\r\n"
                "help\t: "   "print this help menu" "\r\n"
                "hello\t: "  "print Hello World!"   "\r\n"
                "hwinfo\t: " "print hardware info"  "\r\n"
                "ls\t: " "list files in initramfs"  "\r\n"
                "parsedtb\t: " "parse devicetree blob (dtb)"  "\r\n"
                "reboot\t: " "reboot the device"    "\r\n"
                "sw_timer\t: " "turn on/off timer debug info" "\r\n"
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
    BCM2837_reset(10);
}

static void cmd_sw_timer(void)
{
    timer_switch_info();
}

static void cmd_ls(void)
{
    cpio_ls(_initramfs);
}

static void cmd_cat(char *filename)
{
    cpio_cat(_initramfs, filename);
}

static void cmd_exec(char *filename)
{
    char *mem;
    char *user_sp;

    mem = cpio_load_prog(_initramfs, filename);

    if (mem == NULL) {
        return;
    }

    // TODO: Set stack of user program properly
    user_sp = (char *)0x10000000;

    exec_user_prog(mem, user_sp);
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
        if (!strncmp("alloc", shell_buf, 5)) {
            if (cmd_len >= 7) {
                cmd_alloc(&shell_buf[6]);
            }
        } else if (!strcmp("help", shell_buf)) {
            cmd_help();
        } else if (!strcmp("hello", shell_buf)) {
            cmd_hello();
        } else if (!strcmp("hwinfo", shell_buf)) {
            cmd_hwinfo();
        } else if (!strcmp("reboot", shell_buf)) {
            cmd_reboot();
        } else if (!strcmp("sw_timer", shell_buf)) {
            cmd_sw_timer();
        } else if (!strcmp("ls", shell_buf)) {
            cmd_ls();
        } else if (!strcmp("parsedtb", shell_buf)) {
            cmd_parsedtb();
        } else if (!strncmp("cat", shell_buf, 3)) {
            if (cmd_len >= 5) {
                cmd_cat(&shell_buf[4]);
            }
        } else if (!strncmp("exec", shell_buf, 4)) {
            if (cmd_len >= 6) {
                cmd_exec(&shell_buf[5]);
            }
        } else {
            // Just echo back
            uart_printf("%s\r\n", shell_buf);
        }
    }
}

static int initramfs_fdt_parser(int level, char *cur, char *dt_strings)
{
    struct fdt_node_header *nodehdr = (struct fdt_node_header *)cur;
    struct fdt_property *prop;

    uint32 tag = fdtn_tag(nodehdr);

    switch (tag) {
    case FDT_PROP:
        prop = (struct fdt_property *)nodehdr;
        if (!strcmp("linux,initrd-start", dt_strings + fdtp_nameoff(prop))) {
            _initramfs = TO_CHAR_PTR(fdt32_ld((fdt32_t *)&prop->data));
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
    _initramfs = NULL;
    parse_dtb(fdt_base, initramfs_fdt_parser);
    if (!_initramfs) {
        uart_printf("[x] Cannot find initrd address!!!\r\n");
    }
}

void start_kernel(char *fdt)
{
    timer_init();

    fdt_base = fdt;

    uart_init();
    uart_printf("[*] fdt base: %x\r\n", fdt_base);
    uart_printf("[*] Kernel\r\n");

    initramfs_init();

    enable_interrupt();

    shell();
}