// Reference: https://github.com/s-matyukevich/raspberry-pi-os
#include "mm.h"

.section ".text.boot"

.globl _start
_start:
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
