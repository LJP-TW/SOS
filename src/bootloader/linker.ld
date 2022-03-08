_bootloader = 0x60000;
_kernel     = 0x80000;
_stack_top  = 0x400000;

SECTIONS
{
  . = _bootloader;
  
  .text.boot : {
    *(.text.boot)
  }

  .text : {
    *(.text)
  }

  .rodata : {
    *(.rodata)
  }

  .data : {
    *(.got)
    *(.got.plt)
    *(.data)
  }

  _sbss = .;
  .bss : {
    *(.bss*)
  }
  _ebss = .;

  . = _stack_top;
  _sstack = .;
}