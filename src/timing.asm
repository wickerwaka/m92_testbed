BITS 16

%macro time_basic 2
global %1
%1:
    push ds
    push es
    push si
    push di
    push bx
    enter 1024, 0
    mov ax, ss
    mov ds, ax
    mov es, ax
    mov si, sp
    mov di, sp

.wait_zero:
    in al, 0x08
    test al, 0xff
    jnz .wait_zero
    mov cx, 0x1000
    jmp .test_loop

align 2
.test_loop:
    %2
    %2
    %2
    %2

    %2
    %2
    %2
    %2

    %2
    %2
    %2
    %2

    %2
    %2
    %2
    %2

    loop .test_loop
    in al, 0x08

    leave
    pop bx
    pop di
    pop si
    pop es
    pop ds
    ret
%endmacro

global loop_test
loop_test:
.wait_zero:
    mov cx, 0x0000
    in al, 0x08
    test al, 0xff
    jnz .wait_zero

.test_loop:
    nop
    loop .test_loop
    in al, 0x08

    ret

time_basic just_nop, nop
time_basic dec_ax, { dec ax }
time_basic dec_al, { dec al }
time_basic dec_mem2, { dec word [di] }
time_basic dec_mem3, { dec word [di+4] }
time_basic dec_mem4, { dec word [di+256] }
time_basic dec_mem5, { dec word ss:[di+256] }
time_basic dec_mem6, { lock dec word ss:[di+256] }
time_basic mov_reg_reg, { mov ax, dx }
time_basic mov_reg_mem, { mov ax, [di] }
time_basic mov_mem_reg, { mov [di], ax }
time_basic mov_from_mem, { mov ax, [di + 4] }
time_basic mov_to_mem, { mov [di + 4], ax }
time_basic mov_from_mem_seg, { mov ax, ss:[di + 4] }
time_basic mov_to_mem_seg, { mov ss:[di + 4], ax }
time_basic add_mem, { add [di], ax }
time_basic add_mem_seg, { add ss:[di], ax }
time_basic ror0, { ror dx, 0 }
time_basic ror1, { ror dx, 1 }
time_basic ror2, { ror dx, 2 }
time_basic ror3, { ror dx, 3 }
time_basic ror4, { ror dx, 4 }
time_basic ror5, { ror dx, 5 }
time_basic ror6, { ror dx, 6 }
time_basic ror7, { ror dx, 7 }
time_basic ror8, { ror dx, 8 }
time_basic ror9, { ror dx, 9 }
time_basic ror10, { ror dx, 10 }
time_basic ror11, { ror dx, 11 }
time_basic ror12, { ror dx, 12 }
time_basic ror13, { ror dx, 13 }
time_basic ror14, { ror dx, 14 }
time_basic ror15, { ror dx, 15 }
time_basic ror16, { ror dx, 16 }
time_basic rorc80, { rcr dx, 80 }



