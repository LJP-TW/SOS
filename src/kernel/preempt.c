#include <preempt.h>
#include <task.h>
#include <current.h>

void preempt_disable(void)
{
    uint32 daif;

    daif = save_and_disable_interrupt();

    current->preempt += 1;

    restore_interrupt(daif);
}

void preempt_enable(void)
{
    uint32 daif;

    daif = save_and_disable_interrupt();

    current->preempt -= 1;

    restore_interrupt(daif);
}