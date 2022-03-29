// See include/kernel/exec.h for function declaration

.globl exec_user_prog
exec_user_prog:
    // Set exception return address
    msr elr_el1, x0

    // Set user stack
    msr sp_el0, x1

    // Disable interrupt ({D, A, I, F} = 1 (masked))
    // EL0 ({M[3:0]} = 0)
    mov x0, 0x3c0
    msr spsr_el1, x0
    
    // return to EL0
    eret