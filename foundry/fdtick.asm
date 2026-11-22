; Aztec near-call entry. IBM PC BIOS tick low word on CP/M-86 BDOS 2.2.
codeseg segment para public 'code'
        assume cs:codeseg
        public fdtick_
fdtick_ proc near
        push bx
        push si
        push di
        push es
        xor ah,ah
        int 1ah
        mov ax,dx
        pop es
        pop di
        pop si
        pop bx
        ret
fdtick_ endp
codeseg ends
        end
