#include <timer.h>
#include <utils.h>
#include <mini_uart.h>
#include <list.h>
#include <bitops.h>
#include <irq.h>

#define CORE0_TIMER_IRQ_CTRL 0x40000040
#define CORE0_IRQ_SOURCE 0x40000060

#define TIMER_PROC_NUM 32

static void timer_irq_handler(void *);
static void timer_irq_fini(void);

typedef struct {
    // cb(args)
    void (*cb)(void *);
    void *args;
    uint32 remain_time;
    struct list_head list;
} timer_proc;

typedef struct {
    struct list_head head;
    int size;
} timer_meta;

/* Used to store procudures that will be called after a period of time. */
timer_proc t_procs[TIMER_PROC_NUM];

/* Metadata of t_procs.
 * The linked list is sorted by remain_time.
 */
timer_meta t_meta;

/* This variable has 32 bits, n-th bit stands for the status of t_procs[n].
 * If n-th bit is 1, it means t_procs[n] is available.
 * If n-th bit is 0, it means t_procs[n] is unavailable.
 */
uint32 t_status;

/* When setting a timer, this variable will store the interval.
 * It is 0 when timer is not set.
 */
uint32 t_interval;

int timer_show_enable;
uint64 timer_boot_cnt;

static void timer_enable()
{
    // Enable core0 cntp timer
    put32(CORE0_TIMER_IRQ_CTRL, 2);
}

static void timer_disable()
{
    // Disable core0 cntp timer
    put32(CORE0_TIMER_IRQ_CTRL, 0);
}

static timer_proc *tp_alloc()
{
    uint32 daif;
    uint32 idx;

    daif = save_and_disable_interrupt();

    idx = ffs(t_status);

    if (idx == 0) {
        restore_interrupt(daif);

        return NULL;
    }

    t_status &= ~(1 << (idx - 1));

    restore_interrupt(daif);

    return &t_procs[idx - 1];
}

static void tp_release(timer_proc *tp)
{
    if (!tp) {
        return;
    }

    uint32 idx = get_elem_idx(tp, t_procs);

    t_status |= (1 << idx);
}

/*
 * Need to disable interrupt before calling this function.
 */
static void timer_update_remain_time()
{
    timer_proc *iter;
    int32 cntp_tval_el0;
    uint32 diff;

    if (!t_meta.size) {
        return;
    }

    cntp_tval_el0 = read_sysreg(cntp_tval_el0);
    diff = t_interval - cntp_tval_el0;
    t_interval = cntp_tval_el0;

    list_for_each_entry(iter, &t_meta.head, list) {
        if (diff > iter->remain_time) {
            iter->remain_time = 0;
        } else {
            iter->remain_time -= diff;
        }
    }
}

/*
 * Return 1 if @tp was inserted at the first position, otherwise return 0.
 */
static int tp_insert(timer_proc *tp)
{
    uint32 daif;
    timer_proc *iter;
    int first;

    daif = save_and_disable_interrupt();

    // Update remain_time
    timer_update_remain_time();

    // Insert tp
    first = 1;
    list_for_each_entry(iter, &t_meta.head, list) {
        if (tp->remain_time < iter->remain_time) {
            break;
        }
        first = 0;
    }

    list_add_tail(&tp->list, &iter->list);

    t_meta.size += 1;

    restore_interrupt(daif);

    return first;
}

/*
 * Set timer
 */
static void timer_set()
{
    uint32 daif;
    timer_proc *tp;

    daif = save_and_disable_interrupt();

    if (!t_meta.size) {
        restore_interrupt(daif);

        return;
    }
    
    tp = list_first_entry(&t_meta.head, timer_proc, list);

    // Set timer
    t_interval = tp->remain_time;
    write_sysreg(cntp_tval_el0, t_interval);

    restore_interrupt(daif);
}

static void timer_set_boot_cnt()
{
    timer_boot_cnt = read_sysreg(cntpct_el0);
}

static void timer_show_boot_time(void *_)
{
    if (timer_show_enable) {
        uint32 cntfrq_el0 = read_sysreg(cntfrq_el0);
        uint64 cntpct_el0 = read_sysreg(cntpct_el0);
        uart_printf("[Boot time: %lld seconds...]\r\n", 
                    (cntpct_el0 - timer_boot_cnt) / cntfrq_el0);
    }

    timer_add_proc_after(timer_show_boot_time, NULL, 2);
}

void timer_init()
{
    timer_set_boot_cnt();

    INIT_LIST_HEAD(&t_meta.head);
    t_meta.size = 0;

    t_interval = 0;
    t_status = 0xffffffff;

    timer_add_proc_after(timer_show_boot_time, NULL, 2);
}

int timer_irq_check()
{
    uint32 core0_irq_src = get32(CORE0_IRQ_SOURCE);

    if (!(core0_irq_src & 0x02)) {
        return 0;
    }

    timer_disable();
    if (irq_run_task(timer_irq_handler, NULL, timer_irq_fini, 0)) {
        timer_enable();
    }

    return 1;
}

static void timer_irq_handler(void *_)
{
    timer_proc *tp;

    if (!t_meta.size) {
        return;
    }

    // Set next timer before calling any functions which may interrupt
    tp = list_first_entry(&t_meta.head, timer_proc, list);

    list_del(&tp->list);
    t_meta.size -= 1;

    timer_update_remain_time();
    timer_set();

    // Execute the callback function
    (tp->cb)(tp->args);
    tp_release(tp);
}

static void timer_irq_fini(void)
{
    timer_enable();
}

void timer_switch_info()
{
    timer_show_enable = !timer_show_enable;
}

static void timer_add_proc(timer_proc *tp)
{
    int need_update;

    need_update = tp_insert(tp);

    if (need_update) {
        timer_set();
        timer_enable();
    }
}

void timer_add_proc_after(void (*proc)(void *), void *args, uint32 after)
{
    timer_proc *tp;
    uint32 cntfrq_el0;

    tp = tp_alloc();

    if (!tp) {
        return;
    }

    cntfrq_el0 = read_sysreg(cntfrq_el0);

    tp->cb = proc;
    tp->args = args;
    tp->remain_time = after * cntfrq_el0;

    timer_add_proc(tp);
}

void timer_add_proc_freq(void (*proc)(void *), void *args, uint32 freq)
{
    timer_proc *tp;
    uint32 cntfrq_el0;

    tp = tp_alloc();

    if (!tp) {
        return;
    }

    cntfrq_el0 = read_sysreg(cntfrq_el0);

    tp->cb = proc;
    tp->args = args;
    tp->remain_time = cntfrq_el0 / freq;
   
    timer_add_proc(tp);
}