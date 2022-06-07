#include <mini_uart.h>
#include <string.h>
#include <BCM2837.h>
#include <rpi3.h>
#include <cpio.h>
#include <fdt.h>
#include <exec.h>
#include <utils.h>
#include <timer.h>
#include <irq.h>
#include <mm/mm.h>
#include <sched.h>
#include <kthread.h>
#include <current.h>
#include <fs/fsinit.h>

#define BUFSIZE 0x100

static char shell_buf[BUFSIZE];

#define TEST_PTR_CNT 0x10

static uint8 test_ptrs_inused[TEST_PTR_CNT];
static char *test_ptrs[TEST_PTR_CNT];

static void timeout_print(char *str)
{
    uart_printf("%s\r\n", str);

    kfree(str);
}

static void foo(void)
{
    for (int i = 0; i < 10; ++i) {
        uart_sync_printf("Thread id: %d %d\r\n", current->tid, i);
        delay(1000000);
        schedule();
    }
}

static void cmd_alloc(char *ssize)
{
    int size, idx;

    size = atoi(ssize);

    if (size == 0) {
        return;
    }

    for (idx = 0; idx < TEST_PTR_CNT; ++idx) {
        if (!test_ptrs_inused[idx]) {
            break;
        }
    }

    if (idx == TEST_PTR_CNT) {
        return;
    }

    test_ptrs_inused[idx] = 1;
    
    test_ptrs[idx] = kmalloc(size);

    uart_printf("[*] ptrs[%d]: %llx\r\n", idx + 1, test_ptrs[idx]);
}

static void cmd_free(char *sidx)
{
    int idx = atoi(sidx);

    if (idx == 0) {
        return;
    }

    idx -= 1;

    if (!test_ptrs_inused[idx]) {
        return;
    }

    test_ptrs_inused[idx] = 0;

    kfree(test_ptrs[idx]);
}

static void cmd_help(void)
{
    uart_printf(
                "alloc <size>\t: "   "test allocator" "\r\n"
                "exec <filename>\t: " "execute file"  "\r\n"
                "free <idx>\t: " "test allocator"  "\r\n"
                "help\t: "   "print this help menu" "\r\n"
                "hello\t: "  "print Hello World!"   "\r\n"
                "hwinfo\t: " "print hardware info"  "\r\n"
                "parsedtb\t: " "parse devicetree blob (dtb)"  "\r\n"
                "reboot\t: " "reboot the device"    "\r\n"
                "setTimeout <msg> <sec>\t: " 
                    "print @msg after @sec seconds" "\r\n"
                "sw_timer\t: " "turn on/off timer debug info" "\r\n"
                "sw_uart_mode\t: " "use sync/async UART" "\r\n"
                "thread_test\t: " "test kthread" "\r\n"
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

static void cmd_setTimeout(char *msg, char *ssec)
{
    int len;
    char *m;

    len = strlen(msg) + 1;
    m = kmalloc(len);

    if (!m) {
        return;
    }

    memncpy(m, msg, len);

    uart_printf("[*] time: %d\r\n", atoi(ssec));
    timer_add_proc_after((void (*)(void *))timeout_print, m, atoi(ssec));
}

static void cmd_sw_timer(void)
{
    timer_switch_info();
}

static void cmd_sw_uart_mode(void)
{
    int mode = uart_switch_mode();

    if (mode == 0) {
        uart_printf("[*] Use synchronous UART\r\n");
    } else {
        uart_printf("[*] Use asynchronous UART\r\n");
    }
}

static void cmd_exec(char *filename)
{
    // TODO: Add argv & envp
    sched_new_user_prog(filename);
}

static void cmd_parsedtb(void)
{
    fdt_traversal(fdt_base);
}

static void cmd_thread_test(void)
{
    for (int i = 0; i < 3; ++i) {
        kthread_create(foo);
    }
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
        } else if (!strncmp("free", shell_buf, 4)) {
            if (cmd_len >= 6) {
                cmd_free(&shell_buf[5]);
            }
        } else if (!strcmp("help", shell_buf)) {
            cmd_help();
        } else if (!strcmp("hello", shell_buf)) {
            cmd_hello();
        } else if (!strcmp("hwinfo", shell_buf)) {
            cmd_hwinfo();
        } else if (!strcmp("reboot", shell_buf)) {
            cmd_reboot();
        } else if (!strncmp("setTimeout", shell_buf, 10)) {
            char *msg, *ssec;
            
            msg = shell_buf + 10;

            if (*msg != ' ') {
                continue;
            }

            msg++;

            if (!*msg) {
                continue;
            }

            ssec = msg;

            while (*(++ssec)) {
                if (*ssec == ' ') {
                    break;
                }
            }

            if (*ssec != ' ') {
                continue;
            }

            *ssec = 0;

            ssec++;

            if (!*ssec) {
                continue;
            }

            cmd_setTimeout(msg, ssec);
        } else if (!strcmp("sw_timer", shell_buf)) {
            cmd_sw_timer();
        } else if (!strcmp("sw_uart_mode", shell_buf)) {
            cmd_sw_uart_mode();
        } else if (!strcmp("parsedtb", shell_buf)) {
            cmd_parsedtb();
        } else if (!strcmp("thread_test", shell_buf)) {
            cmd_thread_test();
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

static void idle(void)
{
    while (1) {
        kthread_kill_zombies();
        schedule();
    }
}

void start_kernel(char *fdt)
{
    fdt_base = fdt;

    irq_init();
    uart_init();
    initramfs_init();
    mm_init();
    timer_init();
    task_init();
    scheduler_init();
    kthread_early_init();
    fs_init();
    kthread_init();

    uart_printf("[*] fdt base: %x\r\n", fdt_base);
    uart_printf("[*] Kernel start!\r\n");

    // TODO: Remove shell?
    // kthread_create(shell);

    // TODO: Add argv & envp
    // First user program
    sched_new_user_prog("/initramfs/vfs1.img");

    // Enable interrupt from Auxiliary peripherals
    irq1_enable(29);

    enable_interrupt();

    idle();
}