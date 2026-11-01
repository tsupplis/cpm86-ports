#asm
    push    cx
    push    bx
    mov     bx, word ptr [bp+4]   ; argument: loop count (unsigned int)
_dly_outer:
    mov     cx, 0FFFFh
_dly_inner:
    loop    _dly_inner
    dec     bx
    jnz     _dly_outer
    pop     bx
    pop     cx
#endasm
