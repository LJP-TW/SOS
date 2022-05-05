SECTIONS
{
  .start : {
    *(.start)
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
}