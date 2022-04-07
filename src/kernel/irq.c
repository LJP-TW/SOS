#include <irq.h>
#include <mini_uart.h>
#include <utils.h>
#include <timer.h>
#include <BCM2837.h>
#include <list.h>
#include <bitops.h>

#define IRQ_TASK_NUM 32

typedef struct {
    /* cb(args) */
    void (*cb)(void *);
    void *args;
    /* Priority, the lowest priority is 0 */
    uint32 prio;
    uint32 is_running;
    /* Link to next priority */
    struct list_head np_list;
    /* Link all tasks that is same priority */
    struct list_head sp_list;
} irq_task;

irq_task irq_tasks[IRQ_TASK_NUM];

/* Metadata of irq_tasks 
 * This list links tasks with different priorities and is structured 
 * as shown below:
 * @irq_tasks_meta <-> task (prio: 5) <-> task (prio: 2) <-> task (prio: 0)
 *                          ^                  ^                  ^
 *                          |                  |                  |
 *                          v                  v                  v
 *                     task (prio: 5)     task (prio: 2)     task (prio: 0)
 *
 * If a task isn't in the irq_tasks_meta list, its np_list isn't used.
 */
struct list_head irq_tasks_meta;

/* This variable has 32 bits, n-th bit stands for the status of irq_tasks[n].
 * If n-th bit is 1, it means irq_tasks[n] is available.
 * If n-th bit is 0, it means irq_tasks[n] is unavailable.
 */
uint32 irq_tasks_status;

/*
 * Interrupts must be disabled before calling this function.
 */
static irq_task *it_alloc()
{
    uint32 idx;

    idx = ffs(irq_tasks_status);

    if (idx == 0) {
        return NULL;
    }

    irq_tasks_status &= ~(1 << (idx - 1));

    return &irq_tasks[idx - 1];
}

static void it_release(irq_task *it)
{
    if (!it) {
        return;
    }

    uint32 idx = get_elem_idx(it, irq_tasks);

    irq_tasks_status |= (1 << idx);
}

static void it_remove(irq_task *it)
{
    if (!list_empty(&it->np_list)) {
        if (!list_empty(&it->sp_list)) {
            irq_task *prev_np = list_last_entry(&it->np_list, irq_task, np_list);
            irq_task *next_sp = list_first_entry(&it->sp_list, irq_task, sp_list);

            list_del(&it->np_list);
            list_add(&next_sp->np_list, &prev_np->np_list);
        } else {
            list_del(&it->np_list);
        }
    }

    list_del(&it->sp_list);
    it_release(it);
}

/*
 * Interrupts must be disabled before calling this function.
 *
 * Return 1 if @it can preempt the currently running irq_task.
 */
static int it_insert(irq_task *it)
{
    uint64 daif;
    irq_task *iter;
    int preempt;
    int inserted;

    daif = read_sysreg(DAIF);
    disable_interrupt();

    // Insert irq_task
    preempt = 1;
    inserted = 0;

    list_for_each_entry(iter, &irq_tasks_meta, np_list) {
        if (it->prio > iter->prio) {
            list_add_tail(&it->np_list, &iter->np_list);
            inserted = 1;
            break;
        } else if (it->prio == iter->prio) {
            list_add_tail(&it->sp_list, &iter->sp_list);
            preempt = 0;
            inserted = 1;
            break;
        }
        preempt = 0;
    }

    if (!inserted) {
        list_add_tail(&it->np_list, &irq_tasks_meta);
    }

    write_sysreg(DAIF, daif);

    return preempt;
}

static irq_task *it_get_next_task_to_run()
{
    irq_task *it;

    if (list_empty(&irq_tasks_meta)) {
        return NULL;
    }

    it = list_first_entry(&irq_tasks_meta, irq_task, np_list);
    
    if (it->is_running) {
        return NULL;
    }

    return it;
}

/*
 * Run irq_tasks
 */
static inline void it_run()
{
    while (1) {
        irq_task *it = it_get_next_task_to_run();

        if (!it) {
            break;
        }

        it->is_running = 1;

        enable_interrupt();

        (it->cb)(it->args);

        disable_interrupt();

        it_remove(it);
    }
}

void irq_init()
{
    INIT_LIST_HEAD(&irq_tasks_meta);
    irq_tasks_status = 0xffffffff;
}

/*
 * Interrupts must be disabled before calling this function.
 */
void irq_add_tasks(void (*task)(void *), void *args, uint32 prio)
{
    irq_task *it;
    int preempt;

    it = it_alloc();

    if (!it) {
        return;
    }

    it->cb = task;
    it->args = args;
    it->prio = prio;
    it->is_running = 0;
    INIT_LIST_HEAD(&it->np_list);
    INIT_LIST_HEAD(&it->sp_list);
    preempt = it_insert(it);

    if (preempt) {
        it->is_running = 1;

        enable_interrupt();
        
        (task)(args);
        
        disable_interrupt();

        it_remove(it);
    }

    it_run();
}

void irq_handler()
{
    // These check functions may add irq_task.
    timer_irq_check();
    uart_irq_check();
}

void exception_default_handler(uint32 n)
{
    uart_printf("[exception] %d\r\n", n);
}

void irq1_enable(int bit)
{
    put32(IRQ_ENABLE_1_REG, 1 << bit);
}