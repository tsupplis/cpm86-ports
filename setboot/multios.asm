; --------------------------------------------------------------------
; HELLO -- MASM 1.10, 8086/8088.
; One payload: DOS COM or CP/M-86 8080-model CMD.
;
; detect_os returns AL = 0 (DOS) or 1 (CP/M-86); unknown layouts trap.
; This selects the launch personality, not the underlying OS product.
; --------------------------------------------------------------------

program         segment
                assume  cs:program, ds:program, es:program, ss:program
                org     100h

entry:          jmp     near ptr start

OS_DOS          equ     0
OS_CPM          equ     1

hello_dos       db      'Hello world - DOS',13,10,'$'
hello_cpm       db      'Hello world - CP/M-86',13,10,'$'

; CP/M may enter on the loader stack. Reserve and install our own stack.
stack_space     db      512 dup (0)
stack_top       label   word

start:
                cli
                mov     ax,cs
                mov     ss,ax
                mov     sp,offset stack_top
                sti
                cld

; Leave DS untouched until detect_os has checked its entry value.
                call    detect_os
                push    cs
                pop     ds
                cmp     al,OS_DOS
                je      say_dos
                jmp     short say_cpm

say_dos:
                mov     dx,offset hello_dos
                mov     ah,9
                int     21h
                int     20h             ; Also works on DOS 1.x; CS = PSP.

say_cpm:
                mov     dx,offset hello_cpm
                mov     cl,9
                call    cpm_call
                xor     dx,dx
                xor     cx,cx
                call    cpm_call        ; BDOS 0: terminate, release memory.
                jmp     short say_cpm   ; Termination normally never returns.

; ----------------------------------------------------------------------
; Identify the launch interface before using either OS's system calls.
;
; Assumptions: DOS COM or CP/M-86 CMD in the 8080 memory model.
; Checks: entry DS = CS and the matching PSP/base page.
; Only a recognized CP/M base page permits the INT E0h version call.
; No exact version value is required: hybrids report compatibility levels.
; Returns AL; preserves ES. Other registers are not promised preserved.
; The caller restores DS = CS after detection and needs no other values.
; ----------------------------------------------------------------------
detect_os:
                mov     ax,ds
                mov     cx,cs
                cmp     ax,cx
                jne     unknown_os

; DOS PSP: INT 20h at offset 0. CALL 5 details may vary by implementation.
                cmp     word ptr ds:[0],20cdh
                jne     check_cpm
                mov     al,OS_DOS
                ret

; CP/M 8080 base page: model flag at +5, code-group segment at +3.
check_cpm:
                cmp     byte ptr ds:[5],1
                jne     unknown_os
                cmp     word ptr ds:[3],cx
                jne     unknown_os

; A null vector cannot be called. A nonnull vector alone does not prove
; that BDOS exists; the base-page checks above establish the context.
                push    es
                xor     ax,ax
                mov     es,ax
                mov     ax,es:[380h]     ; INT E0h vector: E0h * 4.
                or      ax,es:[382h]
                pop     es
                jz      unknown_os

                mov     cl,12           ; BDOS version, read-only.
                call    cpm_call
                mov     al,OS_CPM
                ret
; Invoke the NMI handler in software. If it returns, trap again.
unknown_os:
                int     2
                jmp     short unknown_os
; No caller needs other registers preserved.
; ES is explicitly protected on every INT E0h call.
cpm_call:
                push    es
                int     0e0h
                pop     es
                ret
program         ends
                end     entry
