_stack_top      = 0xffff000000400000;
_PA_stack_top   = 0x0000000000400000;
_early_mem_base = 0xffff000007000000;
_early_mem_end  = 0xffff000007ffffff;

SECTIONS
{
  . = 0xffff000000080000;
  
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
    *(.data*)
  }

  _sbss = .;
  .bss : {
    *(.bss*)
  }
  _ebss = .;
  
  . = _stack_top;
  _sstack = .;
}