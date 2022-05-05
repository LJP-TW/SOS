#include <mode_switch.h>
#include <signal.h>
#include <utils.h>

void exit_to_user_mode(trapframe regs)
{
    enable_interrupt();

    handle_signal(&regs);

    disable_interrupt();
}