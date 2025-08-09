/* crt0.S */
    .section .text
    .globl _start
_start:
    /* set up a stack somewhere in RAM */
    la   sp, _stack_top
    /* jump to your C entry point */
    call main
1:   wfi           /* or: j 1b  – loop forever */
