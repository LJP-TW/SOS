// Reference: https://github.com/s-matyukevich/raspberry-pi-os
#include "mm.h"

.section ".text.boot"

.globl _start
_start:
  // Preserve flattened devicetree pointer
  mov x19, x0

  bl from_el2_to_el1
  // the next instruction runs in EL1

  bl set_exception_vector_table

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

exception_handler:
  save_all
  // Do nothing
  load_all
  eret

l64_syn_eh:
  save_all

  // Get exception class ({EC[31:26]})
  mrs x0, esr_el1
  asr x1, x0, 26

  // SVC instruction execution
  cmp x1, 0x15
  bne l64_syn_eh_end

  bl syscall_handler

l64_syn_eh_end:
  load_all
  eret

.align 11 // vector table should be aligned to 0x800
.global exception_vector_table
exception_vector_table:
  // Exception from the current EL while using SP_EL0
  // Synchronous
  bl exception_handler
  .align 7
  
  // IRQ
  bl exception_handler
  .align 7

  // FIQ
  bl exception_handler
  .align 7

  // SError
  bl exception_handler
  .align 7

  // Exception from the current EL while using SP_ELx
  // Synchronous
  bl exception_handler
  .align 7

  // IRQ
  bl exception_handler
  .align 7

  // FIQ
  bl exception_handler
  .align 7

  // SError
  bl exception_handler
  .align 7

  // Exception from a lower EL and at least one lower EL is AArch64
  // Synchronous
  bl l64_syn_eh
  .align 7
  
  // IRQ
  bl exception_handler
  .align 7

  // FIQ
  bl exception_handler
  .align 7

  // SError
  bl exception_handler
  .align 7

  // Exception from a lower EL and all lower ELs are AArch32
  // Synchronous
  bl exception_handler
  .align 7

  // IRQ
  bl exception_handler
  .align 7

  // FIQ
  bl exception_handler
  .align 7

  // SError
  bl exception_handler
  .align 7