.code16

.section .entry.text, "ax"
.globl _start
_start:
    // disable interrupts
    cli

    // point ds, es, ss to 0xe0000
    mov $0xe000, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %ss
    // Set stack to end of RAM
    xor %ax, %ax
    mov %ax, %sp

    // Zero out bss data
	cld
	mov    $bss_start,%di
	mov    $bss_end,%cx
	sub    %di,%cx
	xor    %al,%al
    rep    stosb

    // Copy data from 0x10000
    mov    $0x1000, %ax
    mov    %ax, %ds
    mov    $data_start,%si
	mov    %si,%di
	mov    $data_end,%cx
	sub    %di,%cx
    rep movsb
    push   %ss
    pop    %ds

    // Call main
    call main

forever:
    jmp forever


.section .reset.text, "ax"
.globl _reset
_reset:
    ljmp $0x0, $_start
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop

