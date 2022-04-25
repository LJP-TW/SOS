// See include/kernel/exec.c for function declaration

.globl enter_el0_run_user_prog
enter_el0_run_user_prog:
    // Set exception return address
    msr elr_el1, x0

    // Set user stack
    msr sp_el0, x1

    // Enable interrupt ({D, A, I, F} = 0 (unmasked))
    // EL0 ({M[3:0]} = 0)
    mov x0, 0
    msr spsr_el1, x0
    
    // return to EL0
    eret