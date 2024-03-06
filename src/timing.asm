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
    mov es, ax
    mov si, sp
    mov di, sp
    mov ax, 0xb000
    mov ds, ax
    xor bx, bx

align 2
    ror ax, 31
    mov dx, [bx]
    %2
    mov ax, [bx]
    sub ax, dx

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
time_basic ror0, { ror ax, 0 }
time_basic ror1, { ror ax, 1 }
time_basic ror2, { ror ax, 2 }
time_basic ror3, { ror ax, 3 }
time_basic ror4, { ror ax, 4 }
time_basic ror5, { ror ax, 5 }
time_basic ror6, { ror ax, 6 }
time_basic ror7, { ror ax, 7 }
time_basic ror8, { ror ax, 8 }
time_basic ror9, { ror ax, 9 }
time_basic ror10, { ror ax, 10 }
time_basic ror11, { ror ax, 11 }
time_basic ror12, { ror ax, 12 }
time_basic ror13, { ror ax, 13 }
time_basic ror14, { ror ax, 14 }
time_basic ror15, { ror ax, 15 }
time_basic ror16, { ror ax, 16 }
time_basic rorc80, { rcr ax, 80 }



