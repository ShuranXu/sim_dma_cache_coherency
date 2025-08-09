/* link.ld */
ENTRY(_start)
SECTIONS {
  /* load .text at 0x8000_0000 by default */
  . = 0x80000000;
  .text : { *(.text*) }
  .rodata : { *(.rodata*) }
  .data : { *(.data*) }
  .bss  : { *(.bss*) *(COMMON) }
  /* define top-of-stack after all sections */
  _stack_top = . + 0x10000;
}
