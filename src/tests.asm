BITS 16

fill_regs:
    mov ax, 0x5555
    mov bx, 0xdddd
    mov cx, 0x1111
    mov dx, 0x9008
    mov si, 0xc5c5
    mov di, 0x6666
    ret

%macro exercise_arith 1
    call fill_regs
    %1 ax, cx
    %1 cx, cx
    %1 si, di
    %1 di, di
    %1 dx, di
    %1 ax, ax
    %1 dl, al
    %1 cl, dh
    %1 ah, ah
%endmacro

%macro exercise_shift 1
    call fill_regs
    %1 ax, 3
    %1 cx, 5
    %1 si, 8
    %1 di, 11
    %1 dx, 4
    %1 ax, 2
    %1 ax, 1
    %1 si, 1
    %1 di, 1
    %1 dx, 1
    %1 ax, 1
    %1 al, 1
    %1 ch, 1
    %1 bl, 1
    %1 dh, 1 
    mov cl, 3
    %1 ax, cl
    %1 si, cl
    %1 di, cl
    %1 dx, cl
    %1 ax, cl
    %1 al, cl
    %1 ch, cl
    %1 bl, cl
    %1 dh, cl 
%endmacro

exercise_stack:
    call fill_regs
    pusha
    push ax
    push dx
    popa
    pop cx
    pop bx
    ret

exercise_mulu:
    call fill_regs
    mul cl
    mul dl
    mul si
    mul di
    ret

exercise_mul:
    call fill_regs
    imul cx
    imul cx, dx, 6
    imul ax, di, -9
    imul di, 42
    imul bx, 1023
    ret

exercise_string
    cld
    mov di, 0x1000
    mov ax, 0xf001
    mov cx, 0x10
    rep stosw

    mov si, 0x1000
    xor ax, ax
    mov cx, 0x10
    rep lodsw
    ret


global exercise_ops
exercise_ops:
    exercise_arith add
    exercise_arith sub
    exercise_arith sbb
    exercise_arith adc
    exercise_arith or
    exercise_arith and
    exercise_arith xor
    exercise_arith cmp
    exercise_arith test
    exercise_shift rol
    exercise_shift ror
    exercise_shift rcl
    exercise_shift rcr
    exercise_shift shl
    exercise_shift sal
    exercise_shift shr

    exercise_arith xchg

    call exercise_stack
    call exercise_mulu
    call exercise_mul

    call exercise_string


    mov dx, 0xdead
    mov al, 0xff
    out dx, al
    ret

