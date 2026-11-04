; parses filename string to file control block
;---------------------------------------------------------------------
;  entry  : ds:bx = offset of buffer holding filename strings
;         : ds:dx = offset of FCB to be initialised
;---------------------------------------------------------------------
;  exit   : ds:bx = offset of buffer holding filename strings
;         : ds:dx = offset of initialised FCB
;         : ds:si = offset of end of last character of filename + 1
;         :    al = 0 = no error
;         :         1 = drive reference error
;         :         2 = filename error
;         :         3 = file extension error
;         ;    ah = 0 = no wildcard characters
;         ;         1 = wildcard characters
;---------------------------------------------------------------------

filename:
;
; initialise new filename (FCB + 1) with spaces

	push	es			; save es
	cld
	mov	si,bx			; si=offset filename string
	mov	di,dx			; di=offset FCB
	mov	ax,ds
	mov	es,ax			; make es=ds
	xor	ax,ax			; zero ax (clear ah for final return)
	stosb				; init FCB drive reference

	mov	al,020h			; space characrter
	mov	cx,0bh
	rep	stosb			; fill filename & type with spaces

	mov	di,dx			; reset di to FCB offset

;---------------------------------------------------------------------

fscan_drive:
;
;scan for drive reference

	cmp	byte ptr 1[bx],':'
	jz	fscan_drive_ref		; colon = drive reference
	inc	di			; point di to FCB name field
	jmp	fscan_name		; jump to name scan

fscan_drive_ref:
;	
	lodsb
	cmp	al,'Z'			; could be lower case
	jle	fscan_drive_cont
	and	al,05fh			; make upper case

fscan_drive_cont:
;
	cmp	al,'A'
	jl	fscan_drive_error
	cmp	al,'P'
	jg	fscan_drive_error
	sub	al,040h			; make range 1 - 16 (A=1 .. P=16)
	stosb				; put drive ref in FCB
	inc	si			; point si past colon
	jmps	fscan_name

fscan_drive_error:
;
	mov	al,01h			; drive error
	jmp	fscan_return

;---------------------------------------------------------------------

fscan_name:
;
;scan filename
	
	mov	cx,08h

fscan_name_loop:
;
	lodsb				; get character
	or	al,al			;end of filename ?
	jz	fscan_name_exit
	cmp	al,' '			; end of filename ?
	jz	fscan_name_exit
	cmp	al,'.'			; end of filename ?
	jz	fscan_name_end
	cmp	al,'?'			; single wild card ?
	jnz	fscan_name_cont
	mov	ah,01h			; indicate wild characters

fscan_name_cont:
;
	cmp	al,'*'			; wild card ?
	jz	fscan_name_wild
	call	fcheck			; validate character
	jc	fscan_name_error	; invalid if carry set
	stosb				; store in fcb
	loop	fscan_name_loop
	jmps	fscan_name_finished

fscan_name_exit:
;
	cmp	cx,08h			; any characters processed?
	jz	fscan_name_error
	jmp	fscan_complete

fscan_name_finished:
;
	lodsb				; get termination character
	or	al,al			; end of filename ?
	jz	fscan_complete
	cmp	al,' '			; end of filename ?
	jz	fscan_complete
	cmp	al,'.'			; end of filename ?
	jz	fscan_name_end
	jmps	fscan_name_error	; error if any other character


fscan_name_end:
;
	cmp	cx,08h			; any characters processed ?
	jl	fscan_type

fscan_name_error:
;
	mov	al,02h			; filename error
	jmp	fscan_return

fscan_name_wild:
;
	mov	al,'?'
	rep	stosb			; fill remainder of filename
	mov	ah,01h			; indicate wild characters
	lodsb				; get next character
	or	al,al			; end of filename ?
	jz	fscan_complete
	cmp	al,' '			; end of filename ?
	jz	fscan_complete
	cmp	al,'.'			; filename - filetype seperator
	jnz	fscan_name_error

;---------------------------------------------------------------------

fscan_type:
;
; scan filetype

	mov	cx,03h
	mov	di,dx
	add	di,09h

fscan_type_loop:
;
	lodsb				; get character
	or	al,al			; end of filetype ?
	jz	fscan_complete
	cmp	al,' '			; end of filetype ?
	jz	fscan_complete
	cmp	al,'.'			; no more '.' allowed
	jz	fscan_type_error
	cmp	al,'?'			; single wild card ?
	jnz	fscan_type_cont
	mov	ah,01h			; indicate wild characters

fscan_type_cont:
;
	cmp	al,'*'
	jz	fscan_type_wild
	call	fcheck			; validate character
	jc	fscan_type_error	; invalid if carry set
	stosb				; store in fcb
	loop	fscan_type_loop
	jmps	fscan_type_term

fscan_type_wild:
;
	mov	al,'?'
	rep	stosb			; fill remainder of filetype
	mov	ah,01h			; indicate wild characters

fscan_type_term:
;
	lodsb				; check termination character
	or	al,al			; end of filetype ?
	jz	fscan_complete
	cmp	al,' '			; end of filetype ?
	jz	fscan_complete

fscan_type_error:
;
	mov	al,03h			; filetype error
	jmp	fscan_return

;---------------------------------------------------------------------

fscan_complete:
;
	xor	al,al			; no error

fscan_return:
;
	pop	es			; restore es
	ret


;---------------------------------------------------------------------
; Check character in al is legal filename character
;---------------------------------------------------------------------
;  entry  : al=character
;  exit   : al=character
;         : carry set     = invalid filename character
;         : carry not set = valid filename character
;---------------------------------------------------------------------

fcheck:
;
	push	bx
	push	cx			; save registers

	cmp	al,'Z'			; could be lower case
	jle	fcheck_upper
	and	al,05fh			; make upper case

fcheck_upper:
;
	cmp	al,' '			; less than space?
	jl	fcheck_error
	cmp	al,'Z'			; greater than 'Z'?
	jg	fcheck_error
	mov	cx,06h			; number of elements in fcheck_tbl
	mov	bx,offset fcheck_tbl

fcheck_loop:
;
	cmp	al,[bx]
	jz	fcheck_error
	inc	bx
	loop	fcheck_loop
	clc				; clear carry (character valid)
	jmps	fcheck_return

fcheck_error:
;
	stc	     			; set carry (character invalid)

fcheck_return:
;
	pop	cx
	pop	bx			; restore registers
	ret

;---------------------------------------------------------------------
; data required in code segment
;---------------------------------------------------------------------

cpmver	db	0			; cpm version flag for bios calls

;---------------------------------------------------------------------
