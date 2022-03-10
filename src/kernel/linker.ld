_stack_top = 0x400000;
_smem      = 0x7000000;
_emem      = 0x7ffffff;
_initramfs = 0x8000000;

SECTIONS
{
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