// Reference: https://github.com/s-matyukevich/raspberry-pi-os
#include "mm.h"

.section ".text.boot"

.globl _start
_start:
  // Check CPU id
  mrs x0, mpidr_el1
  and x0, x0, #0xff

  // Hang other CPUs
  cbnz x0, proc_hang

  // Initialize bss
  ldr x0, =_sbss
  ldr x1, =_ebss
  sub x1, x1, x0
  bl memzero

  // Initialize stack
  ldr x0, =_stack_top
  mov sp, x0
  bl start_kernel

  // Should never return
proc_hang:
  b proc_hang
