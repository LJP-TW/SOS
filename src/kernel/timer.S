#define CORE0_TIMER_IRQ_CTRL 0x40000040

.globl enable_core0_timer
enable_core0_timer:
  // Enable cntp timer (EL1 physical timer)
  mov x0, 1
  msr cntp_ctl_el0, x0
  
  // Set expired time
  mrs x0, cntfrq_el0
  msr cntp_tval_el0, x0

  // Unmask timer interrupt (Enable nCNTPNSIRQ IRQ control)
  mov x0, 0x02
  ldr x1, =CORE0_TIMER_IRQ_CTRL
  str w0, [x1]

  ret