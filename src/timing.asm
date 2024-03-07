BITS 16

align 256
global small_mov_loop
small_mov_loop:
    mov bx, 0xfff0
    mov dx, 0xdead
    jmp .inner_loop
align 16
.inner_loop:
    mov di, 0xfff2
    mov si, 0xfff2
    rcl ax, 31
    rcl ax, 31
    rcl ax, 31
    rcl ax, 31

    stosb
    stosb
    stosb
    stosb
    lodsb
    lodsb
    lodsb
    lodsb

    mov [bx], ax
    mov [bx], ax
    mov [bx], ax
    mov [bx], ax

    mov [bx], ax
    mov [bx], ax
    mov [bx], ax
    mov [bx], ax

    mov [bx], ax
    mov [bx], ax
    mov [bx], ax
    mov [bx], ax



    out dx, al

    jmp .inner_loop

align 256
global odd_mov_loop
odd_mov_loop:
    mov bx, 0xfff0
    mov dx, 0xdead
    mov cl, 31
    jmp .inner_loop
align 16
.inner_loop:
    ; fill prefetch
    rcl ax, cl
    rcl ax, cl
    rcl ax, cl
    rcl ax, cl

    mov [bx+2], ax
    mov [bx+4], ax
    mov [bx+6], ax
    mov [bx+8], ax

    mov [bx+2], ax
    mov [bx+4], ax
    mov [bx+6], ax
    mov [bx+8], ax

    mov [bx+2], ax
    mov [bx+4], ax
    mov [bx+6], ax
    mov [bx+8], ax

    mov [bx+2], ax
    mov [bx+4], ax
    mov [bx+6], ax
    mov [bx+8], ax

    mov [bx+2], ax
    mov [bx+4], ax
    mov [bx+6], ax
    mov [bx+8], ax

    out dx, al

    jmp .inner_loop

align 256
global push_all_loop
push_all_loop:
    mov cl, 31
    jmp .inner_loop
align 16
.inner_loop:
    ; fill prefetch
    rcl ax, cl
    rcl ax, cl

    pusha
    popa
    
    nop
    rcl ax, cl

    pusha
    popa

    nop
    rcl ax, cl
    push ax
    nop
    rcl ax, cl
    pop ax

    out dx, al

    jmp .inner_loop


align 256
global prefetch_loop
prefetch_loop:
    mov bx, 0xfff0
    mov dx, 0xdead
    mov cl, 31
    jmp .pre_nop
align 16

align 2
.pre_nop:
    rcl ax, cl
    nop
    nop
    jmp .pre_mov2

align 2
.pre_mov2:
    rcl ax, cl
    mov bx, bx
    jmp .pre_mov3

align 2
.pre_mov3:
    rcl ax, cl
    mov byte [bx], 0xee
    jmp .pre_mov4

align 2
.pre_mov4:
    rcl ax, cl
    mov word [bx], 0xeeee
    jmp .pre_mov5

align 2
.pre_mov5:
    rcl ax, cl
    mov word [bx+2], 0xeeee
    jmp .pre_mov6

align 2
.pre_mov6:
    rcl ax, cl
    mov word [bx+2000], 0xeeee
    jmp .pre_mov3_seg

align 2
.pre_mov3_seg:
    rcl ax, cl
    mov byte es:[bx], 0xee
    jmp .finish

align 2
.finish:
    out dx, al

    jmp .pre_nop


align 256
global prefetch_loop_mem
prefetch_loop_mem:
    mov bx, 0xfff0
    mov di, bx
    mov dx, 0xdead
    mov cl, 31
    jmp .pre_nop
align 16

align 2
.pre_nop:
    rcl word [di], cl
    nop
    nop
    jmp .pre_mov2

align 2
.pre_mov2:
    rcl word [di], cl
    mov bx, bx
    jmp .pre_mov3

align 2
.pre_mov3:
    rcl word [di], cl
    mov byte [bx], 0xee
    jmp .pre_mov4

align 2
.pre_mov4:
    rcl word [di], cl
    mov word [bx], 0xeeee
    jmp .pre_mov5

align 2
.pre_mov5:
    rcl word [di], cl
    mov word [bx+2], 0xeeee
    jmp .pre_mov6

align 2
.pre_mov6:
    rcl word [di], cl
    mov word [bx+2000], 0xeeee
    jmp .pre_mov3_seg

align 2
.pre_mov3_seg:
    rcl word [di], cl
    mov byte es:[bx], 0xee
    jmp .finish

align 2
.finish:
    out dx, al

    jmp .pre_nop


align 256
global nop_fetching
nop_fetching:
    mov bx, 0xfff0
    mov dx, 0xdead
    jmp .start
.start:
align 8
    ror    ax,0x1f
    mov    cx,[bx]
    nop
    mov    ax,[bx]
    sub    ax,cx

    out dx, al

    jmp .start



%macro time_basic 2
.%1:
align 2
    ror ax, 11
    mov dx, [bx]
    %2
    mov ax, [bx]
    sub ax, dx
    mov dx, 0xdeee
    out dx, ax

%endmacro

align 256
global time_basic_ops
time_basic_ops:
    enter 1024, 0
    mov ax, ss
    mov es, ax
    mov si, sp
    mov di, sp

    mov ax, 0xb000
    mov ds, ax
    xor bx, bx

.start:
    time_basic just_nop, nop
    time_basic dec_ax, { dec ax }
    time_basic dec_al, { dec al }
    time_basic dec_mem2, { dec word [di] }
    time_basic dec_mem3, { dec word [di+4] }
    time_basic dec_mem4, { dec word [di+256] }
    time_basic dec_mem5, { dec word ss:[di+256] }
    ;time_basic dec_mem5l, { lock dec word [di+256] }
    time_basic dec_mem6, { lock dec word ss:[di+256] }
    time_basic mov_reg_reg, { mov ax, dx }
    time_basic mov_reg_mem, { mov ax, [di] }
    ;time_basic mov_reg_mem_lock, { lock mov ax, [di] }
    time_basic mov_mem_reg, { mov [di], ax }
    ;time_basic mov_mem_reg_lock, { lock mov [di], ax }
    time_basic mov_from_mem, { mov ax, [di + 4] }
    time_basic mov_to_mem, { mov [di + 4], ax }
    time_basic mov_from_mem_seg, { mov ax, ss:[di + 4] }
    time_basic mov_to_mem_seg, { mov ss:[di + 4], ax }
    time_basic add_mem, { add [di], ax }
    ;time_basic add_meml, { lock add [di], ax }
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

    mov dx, 0xdead
    out dx, ax
    jmp .start
