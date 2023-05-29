MEMORY
{
  rom      (rx)  : ORIGIN = 0x00000000, LENGTH = 0x00040000
  ram      (rwx) : ORIGIN = 0x00100000, LENGTH = 0x00010000
}

STACK_SIZE = 0x2000;

/* Section Definitions */
SECTIONS
{
    .text :
    {
        KEEP(*(.vectors .vectors.*));
        *(.eh_frame);
        *(.text .text.*);
        *(.rodata .rodata.*);
        . = ALIGN(2);
        _etext = .;
    } > rom

    . = .;

    _etext_chk = .;

    /* .bss section which is used for uninitialized data */
    .bss (NOLOAD) :
    {
        _sbss = . ;
        *(.bss .bss.*)
        *(COMMON)
        _ebss = . ;
    } > ram

    .data :
    {
        _sdata = .;
        *(.data*);
        _edata = .;
    } > ram AT >rom

    /* stack section */
    .stack (NOLOAD):
    {
        . = ALIGN(2);
        _sstack = .;
        . = . + STACK_SIZE;
        . = ALIGN(2);
        _estack = .;
    } > ram

    . = ALIGN(2);
    _end = . ;
}