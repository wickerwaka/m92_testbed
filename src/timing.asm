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

time_basic just_nop, nop
time_basic dec_ax, { dec ax }
time_basic dec_al, { dec al }
time_basic mov_reg_reg, { mov ax, dx }
time_basic mov_mem_reg, { mov ax, [di] }
time_basic mov_reg_mem, { mov [di], ax }
time_basic mov_from_mem, { mov ax, [di + 4] }
time_basic mov_to_mem, { mov [di + 4], ax }
time_basic mov_from_mem_seg, { mov ax, ss:[di + 4] }
time_basic mov_to_mem_seg, { mov ss:[di + 4], ax }
time_basic add_mem, { add [di], ax }
time_basic add_mem_seg, { add ss:[di], ax }
time_basic ror0, { ror dx, 0 }
time_basic ror1, { ror dx, 1 }
time_basic ror2, { ror dx, 2 }
time_basic ror4, { ror dx, 4 }
time_basic ror8, { ror dx, 8 }
time_basic rorc80, { rcr dx, 80 }



