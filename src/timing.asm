BITS 16

%macro time_basic 2
global %1
%1:
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
    ret
%endmacro

time_basic just_nop, nop
time_basic ror7, { ror dx, 7 }
time_basic rorc80, { rcr dx, 80 }
time_basic mov_from_mem, { mov ax, ss:[0x8000] }
time_basic mov_to_mem, { mov ss:[0x8000], ax }
time_basic add_mem, { add ss:[0x8000], ax }



