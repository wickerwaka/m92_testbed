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

    enter 16, 0
    leave

    enter 8, 1
    leave

    enter 32, 8
    leave

    ret


exercise_mem:
    enter 32, 0
    mov word [bp - 2], 0x8811
    mov byte [bp - 3], 0x22
    mov byte [bp - 4], 0x33
    mov byte [bp - 5], 0x44
    mov word [bp - 7], 0x5566

    mov ax, [bp - 2]
    mov ax, [bp - 4]
    mov al, [bp - 2]
    mov al, [bp - 1]
    mov al, [bp - 3]
    mov al, [bp - 4]
    mov ax, [bp - 3]
    mov ax, [bp - 5]
    mov al, [bp - 6]
    mov al, [bp - 7]
    mov ax, [bp - 7]
    leave
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

exercise_div:
    mov ax, 0x1000
.divloop1:
    xor dx, dx
    mov cx, 0x3
    div cx
    cmp ax, 0
    jne .divloop1

    ret 


exercise_string:
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

exercise_dec:
    mov ax, 0x20
.loop1:
    dec ax
    jne .loop1

.loop2:
    dec al
    jne .loop2

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
    call exercise_div

    call exercise_string

    call exercise_mem

    call exercise_dec


    mov dx, 0xdead
    mov al, 0xff
    out dx, al
    ret

