CPU 186
BITS 16
ORG 0

CODE_SEG equ 0x0000
DATA_SEG equ 0x0000

IDB equ 0xfff
PRC equ 0xfeb
PMC1 equ 0xf0a
EXIC0 equ 0xf4c
EXIC1 equ 0xf4d
EXIC2 equ 0xf4e
RFM equ 0xfe1
WTC equ 0xfe8

;
; DEFINE SECTIONS
; Must come first in this file, no includes before it


; Code, 32KB max, accessed via CS or CODE_SEG. Near calls only
section .text start=0 vstart=0
; Read only data, 32KB max, accessed via CS: or DATA_SEG
section .data start=0x8000 vstart=0x8000
; RAM. Uninitialized. Accessed via RAM_SEG or SS:
section .bss start=0xa0000

; VECTOR TABLE MUST COME FIRST
section .text
	dd 24 dup (generic_handler)
	dd p0_handler
	dd p1_handler
	dd (256 - 26) dup (generic_handler)

;
; MODULES
;

%include "src/util.asm"

;
; MAIN ENTRYPOINT
;
section .text
entry:
	mov ax, 0xa000
	mov ss, ax
	mov ds, ax
	mov sp, stack_start

	call config_system
	call config_timer
	sti
	
.loop_forever:
	jmp .loop_forever

config_system:
	push ds
	mov ax, 0xff00 ; default IDB
	mov ds, ax
	mov ax, 0x9f00
	mov [IDB], ah
	mov ds, ax ; new IDB

	mov byte [PRC], 0x4c
	mov byte [PMC1], 0x80

	mov byte [EXIC0], 0x07
	mov byte [EXIC1], 0x07
	mov byte [EXIC2], 0x47

	mov byte [RFM], 0x00
	mov word [WTC], 0x5555

	mov byte [PRC], 0x0c

	pop ds
	ret

config_timer:
	push dx

	mov dx, 0x1102 ; CLKA2
	call ym_write
	mov dx, 0x1024 ; CLKA1
	call ym_write

	; CLKB
	mov dx, 0x1200
	call ym_write

	mov dx, 0x143f ; 00110101 F Reset, IRQ A EN, LOAD A
	call ym_write

	pop dx
	ret


; dh - reg address, dl - reg value
ym_write:
	push ds
	push ax
	mov ax, 0xa800
	mov ds, ax

.wait_1:
	mov al, [0x42]
	test al, 0x80
	jnz .wait_1

	mov [0x40], dh

.wait_2:
	mov al, [0x42]
	test al, 0x80
	jnz .wait_2

	mov [0x42], dl

	pop ax
	pop ds

	ret

align 4
p0_handler:
	push dx

	mov dx, 0x1435
	call ym_write

    mov dl, ss:[tick_count]
    inc dl
    mov ss:[tick_count], dl

    mov ss:[0x8046], dl

	pop dx
	db 0x0f, 0x92 ; FINI
	iret

align 4
p1_handler:
    push ax
    push dx

    mov al, ss:[0x8044]
    mov ss:[0x8044], al
    test al, 0x80
    jnz .update_timer

    mov ss:[low_n], al

    jmp .fini

.update_timer:
    and ax, 0x007f
    shl ax, 7
    or al, ss:[low_n]
    
    ; CLKA2
    mov dl, al
    and dl, 0x03
    mov dh, 0x11
    call ym_write
    
    ; CLKA1
    shr ax, 2
    mov dl, al
    mov dh, 0x10
    call ym_write

.fini:
    pop dx
    pop ax
	db 0x0f, 0x92 ; FINI
	iret

align 4
generic_handler:
	nop
	iret

;
; RAM accessed via DATA_SEG or ss:
;
section .bss
alignb 2
tick_count: resb 1
low_n: resb 1


stack: resb 256
stack_start:


;
; V35 STARTUP
;

section .text_start start=0x1fff0 vstart=0
startup:
	cli
	jmp CODE_SEG:entry
	db 0, 0, 0, 0, 0
	db 0, 0, 0, 0, 0




