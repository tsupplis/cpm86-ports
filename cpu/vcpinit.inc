; code segment
;---------------------------------------------------------------------

	cseg
	org	0000h

;---------------------------------------------------------------------
; jump past my message. It's in case someone types the .CMD file.
;---------------------------------------------------------------------

	jmps	start

	db	'VCPM NEC-V20 CP/M-80 Emulator for CP/M-86  '
	db	'Version '
	db	ver/10+'0','.',ver mod 10+'0'
	db	'(C) 05-JUNE-2000 Stephen Hunt'

	db	01ah

;---------------------------------------------------------------------
; tell 'em what we are
;---------------------------------------------------------------------

start:
;
;	switch from the CCP stack (96 bytes) to a private stack in the
;	data group; 8080 traps and interrupts also run on it
;
	mov	ax,ds
	cli
	mov	ss,ax
	mov	sp,offset vstack
	sti

	mov	dx,offset startmsg
	call	print_string

	mov	cl,s_bdosver
	call	bdos			; get cpm version
	and	al,0f0h
	mov	cpmver,al		; and save it

;---------------------------------------------------------------------
; test for NEC V20/30 processor
;---------------------------------------------------------------------

test_processor:
;
	mov	ax,0f0fh		; set ax to 0F0Fh

	aad10				; 8088 will merge 0F0Fh to 00FFh
					; where as the V20 will not

	cmp	al,0ffh			; 8088 processor?
	jnz	parse_comline		; no - it must be a V20

	mov	dx,offset err_processor
	jmp	error_exit

;---------------------------------------------------------------------
; parse command line
;---------------------------------------------------------------------

parse_comline:
;
	mov	bx,dma+1
	call	lead_blank		; check through leading blanks
	jnz	parse_comline1		; continue if comline not empty

	mov	dx,offset err_command
	jmp	error_exit		; exit with error message

parse_comline1:
;
	mov	dx,offset fcb0
	call	filename		; parse program filename
	or	ax,ax			; any errors/wildcards
	jz	parse_comline2

	mov	dx,offset err_filename
	jmp	error_exit		; exit with error mesage

parse_comline2:
;
	mov	comline_next,si		; store address of next part of comline
	dec	comline_next		; correct next offset address
	mov	bx,si
	call	lead_blank		; check through leading blanks
	jz	clear_eseg		; jump if parsing complete

parse_comline3:
;
	mov	dx,offset fcb1
	call	filename		; parse fcb1

	mov	bx,si
	call	lead_blank		; check through leading blanks
	jz	clear_eseg		; jump if parsing complete

parse_comline4:
;
	mov	dx,offset fcb2
	call	filename		; parse fcb2

;---------------------------------------------------------------------
; clear the 64k of ESEG memory (fill with 0's)
;---------------------------------------------------------------------

clear_eseg:
;
	xor	di,di			; point to start of eseg
	xor	ax,ax
	mov	cx,0ffffh /2
	cld
	rep	stosw			; fill eseg with chr ax

;---------------------------------------------------------------------
; copy parsed file control blocks to 8080 base page
;---------------------------------------------------------------------

copy_fcbs:
;
	mov	si,offset fcb1
	mov	di,05ch
	mov	cx,0ch
	rep	movsb			; copy fcb1 to 8080 fcb1

	mov	si,offset fcb2
	mov	di,06ch
	mov	cx,0ch
	rep	movsb			; copy fcb2 to 8080 fcb2

;---------------------------------------------------------------------
; copy command line to 8080 command line (in default 8080 dma buffer)
;---------------------------------------------------------------------

copy_comline:
;
	mov	si,comline_next
	mov	bx,dma
	mov	di,bx
	inc	di
	mov	es:byte ptr 0[bx],0

copy_comline1:
;
	lodsb				; get byte from 8086 dma buffer
	or	al,al
	jz	prog_ext_com		; end of command tail (eseg was cleared,
					; so the 8080 tail is already terminated)
	cmp	es:byte ptr 0[bx],07fh	; 8080 tail holds at most 127 chars
	jae	prog_ext_com		; tail full - truncate rather than run
					; past the dma buffer into the program
	stosb				; store in 8080 dma buffer
	inc	es:byte ptr 0[bx]
	jmps	copy_comline1

;---------------------------------------------------------------------
; open '.COM' file
;---------------------------------------------------------------------

prog_ext_com:
;
	mov	byte ptr fcb0 +09h,'C'
	mov	byte ptr fcb0 +0ah,'O'
	mov	byte ptr fcb0 +0bh,'M'

open_prog_com:
;
	mov	dx,offset fcb0
	mov	cl,f_open
	call	bdos			; open program.COM file
	inc	al			; file opened ok?
	jnz	set_dma_segment		; yes -continue

	mov	dx,offset err_open
	jmp	error_exit

;---------------------------------------------------------------------
; set dma segment = ESEG
;---------------------------------------------------------------------

set_dma_segment:
;
	mov	dx,es
	mov	cl,f_dmaseg
	call	bdos			; set dma segment to 8080 segment

;---------------------------------------------------------------------
; read program file into ESEG from offset 0100h
;---------------------------------------------------------------------

read_prog:
;
	mov	currec0,0		; init sequential record number
	mov	dx,0100h		; dma offset - load address

read_prog_loop:
;
	push	dx			; save dma offset
	mov	cl,f_dmaoff		; set dma offset
	call	bdos
	mov	dx,offset fcb0
	mov	cl,f_read		; read sequentially
	call	bdos
	pop	dx			; restore dma offset
	cmp	al,01h			; end of file?
	je	close_file		; close file at eof
	or	al,al			; any other nonzero result is a read error
	jnz	read_error		; abort - a truncated load must not run
	add	dx,080h			; bump dma offset by 80h
	cmp	dx,0fb00h		; (read upto fb80h)
					; NB: this allows 500 records (64000
					; bytes); a 501st record would still
					; fit below the 8080 bios at 0fbe6h
					; but is rejected as too large
	jbe	read_prog_loop		; loop if prog not too big

	mov	dx,offset err_big	; program too big
	jmps	com_size_error

read_error:
;
	mov	dx,offset err_read	; read error
	jmp	error_exit

close_file:
;
	push	dx			; save last dma offset
	mov	dx,offset fcb0
	mov	cl,f_close
	call	bdos
	pop	dx			; restore last dma offset
	cmp	dx,0100h		; was anything loaded
	ja	copy_code
	mov	dx,offset err_small	; program too small

com_size_error:
;
	push	dx
	mov	dx,offset err_size
	call	print_string		; print part of size message
	pop	dx
	jmp	error_exit		; exit with error

;---------------------------------------------------------------------
; copy V20/8080 code to ESEG
;---------------------------------------------------------------------

copy_code:
;
	mov	si,offset v20code	; ds:si
	mov	di,0fbe6h		; es:di
	mov	cx,vcode_size
	rep	movsb			; copy 8080 code to 8080 segment

;---------------------------------------------------------------------
; set up iobyte + default drive
;---------------------------------------------------------------------

set_defaults:
;
	mov	di,iobyte

	mov	cl,s_iobyte
	call	bdos			; get i/o byte
	stosb				; and store in 8080 segment

	mov	cl,drv_get
	call	bdos			; get default drive
	stosb				; and store in 8080 segment

;---------------------------------------------------------------------
; set up special interrupts
;---------------------------------------------------------------------

set_ints:
;
	push	ds			; save ds
	xor	ax,ax
	mov	ds,ax			; ds = 0

	mov	.intemuladr,0fc83h	; setup int 80h (call 8080 @ 0FC83h)
	mov	ax,es
	mov	.intemulseg,ax

	mov	.intbdosadr,offset bdostrap	; setup int 81h (bdos routine)
	mov	ax,cs
	mov	.intbdosseg,ax

	mov	.intbiosadr,offset biostrap	; setup int 82h (bios routine)
	mov	.intbiosseg,ax

	pop	ds			; restore ds

;---------------------------------------------------------------------
; reset dma offset to 080h (ready for cpm-80 programs use)
;---------------------------------------------------------------------

	mov	dx,080h
	mov	cl,f_dmaoff
	call	bdos

;---------------------------------------------------------------------
; break to emulation mode (ds must point to 8080 code segment)
;---------------------------------------------------------------------

execute_cpm80:
;
	push	ds			; save ds
	mov	ax,es
	mov	ds,ax			; ds= 8080 code segment
	sti				; interrupts must be on !

	brkem	intemul			; emulation interrupt

	mov	al,byte ptr .statcode	; get 8086 status flag
	pop	ds			; restore ds

	or	al,al			; did we force 8080 exit?
	jz	execute_terminate	; no - jump

	mov	dx,offset err_bios	; must have been a bios disk call
	call	print_string

execute_terminate:
;
	mov	dx,offset exit80	; tell 'em we've finished
	jmp	error_exit

;---------------------------------------------------------------------
