// Reference: https://github.com/s-matyukevich/raspberry-pi-os
.section ".text.boot"

.globl _start
_start:
  // Preserve flattened devicetree pointer
  mov x19, x0

  bl from_el2_to_el1
  // the next instruction runs in EL1

  bl set_exception_vector_table
  
  bl enable_core0_timer

  // Initialize bss
  ldr x0, =_sbss
  ldr x1, =_ebss
  sub x1, x1, x0
  bl memzero

  // Initialize stack
  ldr x0, =_stack_top
  mov sp, x0
  // Pass flattened devicetree pointer
  mov x0, x19
  bl start_kernel

  // Should never return
proc_hang:
  b proc_hang

from_el2_to_el1:
  // EL1 uses aarch64
  mov x0, (1 << 31)
  msr hcr_el2, x0
  // Disable interrupt ({D, A, I, F} = 1 (masked))
  // EL1h ({M[3:0]} = 5)
  mov x0, 0x3c5
  msr spsr_el2, x0
  msr elr_el2, lr
  // return to EL1
  eret

set_exception_vector_table:
  adr x0, exception_vector_table
  msr vbar_el1, x0
  ret

// save general registers to stack
.macro save_all
  sub sp, sp, 32 * 8
  stp x0, x1, [sp ,16 * 0]
  stp x2, x3, [sp ,16 * 1]
  stp x4, x5, [sp ,16 * 2]
  stp x6, x7, [sp ,16 * 3]
  stp x8, x9, [sp ,16 * 4]
  stp x10, x11, [sp ,16 * 5]
  stp x12, x13, [sp ,16 * 6]
  stp x14, x15, [sp ,16 * 7]
  stp x16, x17, [sp ,16 * 8]
  stp x18, x19, [sp ,16 * 9]
  stp x20, x21, [sp ,16 * 10]
  stp x22, x23, [sp ,16 * 11]
  stp x24, x25, [sp ,16 * 12]
  stp x26, x27, [sp ,16 * 13]
  stp x28, x29, [sp ,16 * 14]
  str x30, [sp, 16 * 15]
.endm

// save exception registers to stack
.macro save_exception_reg
  sub sp, sp, 2 * 8
  mrs x0, elr_el1
  mrs x1, spsr_el1
  stp x0, x1, [sp, 0]
.endm

// load general registers from stack
.macro load_all
  ldp x0, x1, [sp ,16 * 0]
  ldp x2, x3, [sp ,16 * 1]
  ldp x4, x5, [sp ,16 * 2]
  ldp x6, x7, [sp ,16 * 3]
  ldp x8, x9, [sp ,16 * 4]
  ldp x10, x11, [sp ,16 * 5]
  ldp x12, x13, [sp ,16 * 6]
  ldp x14, x15, [sp ,16 * 7]
  ldp x16, x17, [sp ,16 * 8]
  ldp x18, x19, [sp ,16 * 9]
  ldp x20, x21, [sp ,16 * 10]
  ldp x22, x23, [sp ,16 * 11]
  ldp x24, x25, [sp ,16 * 12]
  ldp x26, x27, [sp ,16 * 13]
  ldp x28, x29, [sp ,16 * 14]
  ldr x30, [sp, 16 * 15]
  add sp, sp, 32 * 8
.endm

// load exception registers from stack
.macro load_exception_reg
  ldp x0, x1, [sp, 0]
  msr elr_el1, x0
  msr spsr_el1, x1
  add sp, sp, 2 * 8
.endm

.macro kernel_entry el
  sub sp, sp, 17 * 16
  stp x0, x1, [sp ,16 * 0]
  stp x2, x3, [sp ,16 * 1]
  stp x4, x5, [sp ,16 * 2]
  stp x6, x7, [sp ,16 * 3]
  stp x8, x9, [sp ,16 * 4]
  stp x10, x11, [sp ,16 * 5]
  stp x12, x13, [sp ,16 * 6]
  stp x14, x15, [sp ,16 * 7]
  stp x16, x17, [sp ,16 * 8]
  stp x18, x19, [sp ,16 * 9]
  stp x20, x21, [sp ,16 * 10]
  stp x22, x23, [sp ,16 * 11]
  stp x24, x25, [sp ,16 * 12]
  stp x26, x27, [sp ,16 * 13]
  stp x28, x29, [sp ,16 * 14]

  .if \el == 0
  mrs x0, sp_el\el
  stp x30, x0, [sp, 16 * 15]
  .else
  str x30, [sp, 16 * 15]
  .endif

  mrs x0, elr_el1
  mrs x1, spsr_el1
  stp x0, x1, [sp, 16 * 16]
.endm

.macro kernel_exit el
  ldp x0, x1, [sp, 16 * 16]
  msr elr_el1, x0
  msr spsr_el1, x1

  .if \el ==0
  ldp x30, x0, [sp, 16 * 15]
  msr sp_el\el, x0
  .else
  ldr x30, [sp, 16 * 15]
  .endif

  ldp x28, x29, [sp ,16 * 14]
  ldp x26, x27, [sp ,16 * 13]
  ldp x24, x25, [sp ,16 * 12]
  ldp x22, x23, [sp ,16 * 11]
  ldp x20, x21, [sp ,16 * 10]
  ldp x18, x19, [sp ,16 * 9]
  ldp x16, x17, [sp ,16 * 8]
  ldp x14, x15, [sp ,16 * 7]
  ldp x12, x13, [sp ,16 * 6]
  ldp x10, x11, [sp ,16 * 5]
  ldp x8, x9, [sp ,16 * 4]
  ldp x6, x7, [sp ,16 * 3]
  ldp x4, x5, [sp ,16 * 2]
  ldp x2, x3, [sp ,16 * 1]
  ldp x0, x1, [sp ,16 * 0]
  add sp, sp, 17 * 16

  eret
.endm

exception_handler:
  // Do nothing
  save_all

  mov x0, 0
  bl exception_default_handler

  load_all
  eret

l64_syn_eh:
  kernel_entry 0

  mov x0, sp
  mrs x1, esr_el1
  bl syscall_handler

  mov x0, sp
  bl exit_to_user_mode

  kernel_exit 0

l64_irq_eh:
  kernel_entry 0

  bl irq_handler

  mov x0, sp
  bl exit_to_user_mode

  kernel_exit 0

curr_syn_eh:
  save_all

  mov x0, 1
  bl exception_default_handler

  load_all
  eret

curr_irq_eh:
  kernel_entry 1

  bl irq_handler

  kernel_exit 1

curr_fiq_eh:
  save_all

  mov x0, 2
  bl exception_default_handler

  load_all
  eret

curr_se_eh:
  save_all

  mov x0, 3
  bl exception_default_handler

  load_all
  eret

.align 11 // vector table should be aligned to 0x800
.global exception_vector_table
exception_vector_table:
  // Exception from the current EL while using SP_EL0
  // Synchronous
  b exception_handler
  .align 7
  
  // IRQ
  b exception_handler
  .align 7

  // FIQ
  b exception_handler
  .align 7

  // SError
  b exception_handler
  .align 7

  // Exception from the current EL while using SP_ELx
  // Synchronous
  b curr_syn_eh
  .align 7

  // IRQ
  b curr_irq_eh
  .align 7

  // FIQ
  b curr_fiq_eh
  .align 7

  // SError
  b curr_se_eh
  .align 7

  // Exception from a lower EL and at least one lower EL is AArch64
  // Synchronous
  b l64_syn_eh
  .align 7
  
  // IRQ
  b l64_irq_eh
  .align 7

  // FIQ
  b exception_handler
  .align 7

  // SError
  b exception_handler
  .align 7

  // Exception from a lower EL and all lower ELs are AArch32
  // Synchronous
  b exception_handler
  .align 7

  // IRQ
  b exception_handler
  .align 7

  // FIQ
  b exception_handler
  .align 7

  // SError
  b exception_handler
  .align 7