_stack_top = 0x400000;

SECTIONS
{
  . = 0x0;

  _sbss = .;
  .bss : {
    *(.bss*)
  }
  _ebss = .;

  . = 0x80000;
  
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
    *(.data)
  }
  
  . = _stack_top;
  _sstack = .;
}