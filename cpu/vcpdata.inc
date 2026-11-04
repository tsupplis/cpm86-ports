; data segment
;---------------------------------------------------------------------

	dseg

	org	05ch


fcb0	db	0,'           ',0,0,0,0,0,0
	db	0,0,0,0,0,0,0,0,0,0,0,0,0,0
currec0	db	0				; current record (sequential)
ranrec0	dw	0				; random record (random)
ranovf0	db	0				; random record overflow
;

	org	0100h


; startmsg is a placeholder for the start-up banner. The banner lines
; below are disabled, so only the '$' terminator is left and nothing prints.
startmsg	db	'$'
		;db	'---------------------------------------------',cr,lf
		;db	'VCPM NEC-V20 64K CP/M-80 Emulator for CP/M-86',cr,lf
		;db	'Version '
		;db	ver/10+'0','.',ver mod 10+'0'
		;db	'     (C) 05-JUNE-2000    Stephen Hunt',cr,lf
		;db	'---------------------------------------------',cr,lf

err_processor	db	'VCPM: Requires NEC-V20 processor!',cr,lf,'$'
err_command	db	'VCPM: No command tail provided',cr,lf,'$'
err_filename	db	cr,lf,'VCPM: Invalid filename',cr,lf,'$'
err_open	db	'VCPM: .COM file not found',cr,lf,'$'
err_size	db	cr,lf,'.COM file too $'
err_big		db	'large !',cr,lf,'$'
err_small	db	'small !',cr,lf,'$'
err_bios	db	'VCPM: Unsupported BIOS call',cr,lf,'$'
err_read	db	'VCPM: Error reading .COM file',cr,lf,'$'
exit80		db	'$' ;'VCPM: CP/M-80 emulation terminated',cr,lf,'$'

fcb1	db	0,'           '			; blank fcb1
fcb2	db	0,'           '			; blank fcb2

comline_next	dw	0

fcheck_tbl	db	',:;<=>'		; invalid character table


v20code:
vcode_start		equ	offset $

;---------------------------------------------------------------------
;	V20BIOS 1.3 for CP/M 2.2 8080 emulation
;
;	Copyright (C) 08-AUG-88 Stephen Hunt
;---------------------------------------------------------------------
;---------------------------------------------------------------------
;	8080 bdos trap routine
;---------------------------------------------------------------------

;org	0fbe6h			; so that BIOS starts at 0fc00h
bdos80:
;
db	0C3h,0EDh,0FBh	;jmp	callbdos
db	00h		;nop
db	00h		;nop
db	00h		;nop
db	00h		;nop

callbdos:
;
db	079h		;mov	a,c
db	0B7h		;ora	a
db	0CAh,03h,0FCh	;jz	wboote		; terminate if bdos function 0
db	0EDh,0EDh,081h	;calln	intbdos		; call native 8086 bdos

teststat:
;
db	0F5h		;push	psw		; save register a and flags
db	03Ah,0FFh,0FBh	;lda	statcode	; get 8086 return code in register a
db	0B7h		;ora	a		; return code 0?
db	0C2h,03h,0FCh	;jnz	wboote		; no - terminate emulation
db	0F1h		;pop	psw		; restore register a and flags

db	0C9h		;ret


;statcode				; return status code (at FBFF)
db	0				; zero     = ok
					; non zero = terminate emulation

;---------------------------------------------------------------------
;	8080 bios jump vector for individual subroutines
;---------------------------------------------------------------------

db	0C3h,033h,0FCh 	;jmp	boot		; 00 cold start
wboote:
db	0C3h,033h,0FCh 	;jmp	wboot		; 01 warm start
db	0C3h,035h,0FCh 	;jmp	const		; 02 console status
db	0C3h,03Ah,0FCh 	;jmp	conin		; 03 console character in
db	0C3h,03Fh,0FCh 	;jmp	conout		; 04 console character out
db	0C3h,044h,0FCh 	;jmp	llist		; 05 list character out
db	0C3h,049h,0FCh 	;jmp	punch		; 06 punch character out
db	0C3h,04Eh,0FCh 	;jmp	reader		; 07 reader character out
db	0C3h,053h,0FCh 	;jmp	home		; 08 move head to home position
db	0C3h,058h,0FCh 	;jmp	seldsk		; 09 select disk
db	0C3h,05Dh,0FCh 	;jmp	settrk		; 0a set track number
db	0C3h,062h,0FCh 	;jmp	setsec		; 0b set sector number
db	0C3h,067h,0FCh 	;jmp	setdma		; 0c set dma address
db	0C3h,06Ch,0FCh 	;jmp	read		; 0d read disk
db	0C3h,071h,0FCh 	;jmp	write		; 0e write disk
db	0C3h,076h,0FCh 	;jmp	listst		; 0f return list status
db	0C3h,07Bh,0FCh 	;jmp	sectran		; 10 sector translate

;---------------------------------------------------------------------
;	individual subroutines to perform each bios function
;---------------------------------------------------------------------

;
boot:	; boot & wboot both perform the same function
;
wboot:
;
db	0EDh,0FDh	;retem

const:	; console status, return 0ffh if character ready, 00h if not
;
db	03Eh,02h      	;mvi	a,02h
db	0C3h,07Dh,0FCh 	;jmp	callbios

conin:	; console character into register a
;
db	03Eh,03h      	;mvi	a,03h
db	0C3h,07Dh,0FCh 	;jmp	callbios

conout: ; console character output from register c
;
db	03Eh,04h      	;mvi	a,04h
db	0C3h,07Dh,0FCh 	;jmp	callbios

llist:	; list character from register c
;
db	03Eh,05h      	;mvi	a,05h
db	0C3h,07Dh,0FCh 	;jmp	callbios

punch:	; punch character from register c
;
db	03Eh,06h      	;mvi	a,06h
db	0C3h,07Dh,0FCh 	;jmp	callbios

reader: ; read character into register a from reader device
;
db	03Eh,07h      	;mvi	a,07h
db	0C3h,07Dh,0FCh 	;jmp	callbios

home:	; move to the track 00 position of current drive
;
db	03Eh,08h      	;mvi	a,08h
db	0C3h,07Dh,0FCh 	;jmp	callbios

seldsk:	; select disk given by register C
;
db	03Eh,09h      	;mvi	a,09h
db	0C3h,07Dh,0FCh 	;jmp	callbios

settrk:	; set track given by register c
;
db	03Eh,0Ah      	;mvi	a,0ah
db	0C3h,07Dh,0FCh 	;jmp	callbios

setsec:	; set sector given by register c
;
db	03Eh,0Bh      	;mvi	a,0bh
db	0C3h,07Dh,0FCh 	;jmp	callbios

setdma:	; set dma address given by registers b and c
;
db	03Eh,0Ch      	;mvi	a,0ch
db	0C3h,07Dh,0FCh	;jmp	callbios

read:	; perform read operation
;
db	03Eh,0Dh      	;mvi	a,0dh
db	0C3h,07Dh,0FCh 	;jmp	callbios

write:	; perform a write operation
;
db	03Eh,0Eh      	;mvi	a,0eh
db	0C3h,07Dh,0FCh 	;jmp	callbios

listst:	; return list status (0 if not ready, 1 if ready)
;
db	03Eh,0Fh      	;mvi	a,0fh
db	0C3h,07Dh,0FCh 	;jmp	callbios

sectran:
	; translate the sector given by BC using the
	; translate table given by DE
;
db	03Eh,010h      	;mvi	a,010h
;			;jmp	callbios

callbios:
; execute 8086 bios routines
;
db	0EDh,0EDh,082h	;calln	intbios
db	0C3h,0F5h,0FBh	;jmp	teststat	; test return status and
						; return if ok

;---------------------------------------------------------------------
;	8080 bios initialisation routine
;---------------------------------------------------------------------

;	initialisation code stored in buffer memory (FC83 - FFFF)
;	set parameters and go to 0100h
go100:
db	03Eh,0C3h	;mvi	a,0c3h		; c3 is a jmp instruction
db	032h,00h,00h	;sta	0		; for jmp to wboot
db	021h,03h,0FCh	;lxi	h,wboote	; wboot entry point
db	022h,01h,00h	;shld	1		; set address field for jmp at 0

db	032h,05h,00h	;sta	5		; for jmp to bdos
db	021h,0E6h,0FBh	;lxi	h,bdos80	; bdos entry point
db	022h,06h,00h	;shld	6		; address field of jump at 5 to bdos

db	031h,0FFh,0FFh	;lxi	sp,0ffffh	; set stack to top of 64k

db	021h,03h,0FCh	;lxi	h,wboote	; hl points to wboote offset
db	0E5h		;push	h		; push return address onto stack

db	0C3h,00h,01h	;jmp	0100h		; go to 0100h

;---------------------------------------------------------------------
;	end of NEC-V20 CP/M 2.2 8080 emulation bios
;---------------------------------------------------------------------


vcode_size		equ	offset $ - vcode_start

;---------------------------------------------------------------------
; private 8086 stack (256 bytes), kept after vcode_size so it is not
; copied to the 8080 segment; the trailing db makes GENCMD keep it
;---------------------------------------------------------------------

			rb	256
vstack			equ	$
			db	0



;---------------------------------------------------------------------
; end of VCPM.A86
;---------------------------------------------------------------------

