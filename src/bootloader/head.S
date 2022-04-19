// Reference: https://github.com/s-matyukevich/raspberry-pi-os
.section ".text.boot"

.globl _start
_start:
  // Preserve flattened devicetree pointer
  mov x19, x0

  // Check CPU id
  mrs x0, mpidr_el1
  and x0, x0, #0xff

  // Hang other CPUs
  cbnz x0, proc_hang

  // Get offset
  bl get_offset
_get_offset_ret:
  
  // Relocate bootloader
  b.ne relocate

  // Initialize bss
  adr x0, _sbss
  adr x1, _ebss
  sub x1, x1, x0
  bl memzero

  // Initialize stack
  ldr x0, =_stack_top
  mov sp, x0
  // Pass flattened devicetree pointer
  mov x0, x19
  bl start_bootloader

  // Should never return
proc_hang:
  b proc_hang

get_offset:
  adr x0, _get_offset_ret
  adr x1, _start
  sub x0, x0, x1
  sub x0, lr, x0
  ldr x1, =_bootloader
  subs x0, x0, x1
  ret

relocate:
  mov x2, x0

  // Get Dest
  ldr x0, =_bootloader

  // Get source
  adr x3, _get_offset_ret
  adr x1, _start
  sub x1, x3, x1
  sub x1, lr, x1

  // Get length
  ldr x2, =_bootloader
  ldr x3, =_edata
  sub x2, x3, x2
  bl memncpy

  // Pass flattened devicetree pointer
  mov x0, x19
  ldr x1, =_bootloader
  br x1
  // No return