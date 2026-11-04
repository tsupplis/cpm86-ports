; bdos trap
;---------------------------------------------------------------------

bdostrap:
;
	sti				; re-enable interrupts
	push	bp
	push	es
	push	ds
	cmp	cl,06h			; direct console i/o ?
	jz	bdostrap_06h
	cmp	cl,0bh			; get console status ?
	jz	bdostrap_0bh
	cmp	cl,01bh			; get allocation address ?
	jz	bdostrap_01bh
	cmp	cl,01fh			; get parameters address ?
	jz	bdostrap_01fh

bdos_8086:
;
	int	cpm			; perform bdos interrupt

bdosret:
;
	pop	ds
	pop	es
	pop	bp
	iret

bdostrap_06h:
;
	cmp	dl,0fdh			; direct console i/o
	jnz	bdos_8086		; wait for key press ?

bdostrap_wait:				; MPM-II style !
;
	mov	cl,06h
	mov	dl,0ffh			; console input
	int	cpm
	or	al,al			; anything entered ?
	jz	bdostrap_wait		; no - try again
	jmps	bdosret

bdostrap_0bh:
;
	int	cpm			; get console status
	or	al,al
	jz	bdosret			; 0 = no chr ready
	mov	al,0ffh			; cpm-86 returns 01 for ready
					; change to 0ffh for cpm-80
	jmps	bdosret

bdostrap_01bh:				; get allocation address
;
	mov	di,alloc_tbl		; allocation table
	mov	ax,0100h		; max length 200h bytes
	jmps	bdostrap_get

bdostrap_01fh:				; get parameters address
;
	mov	di,param_tbl		; parameter table
	mov	ax,08h			; max length 10h bytes

bdostrap_get:
;
	push	di			; save destination offset
	push	ax			; save count
	push	es			; save destination segment
	int	cpm
	mov	ax,es
	mov	ds,ax			; ds = table source seg
	pop	es			; es = table dest seg
	pop	cx			; cx = table length
	pop	di			; di = dest offset
	mov	si,bx			; si = source offset
	mov	bx,di			; bx = dest offset (for return (in hl))
	rep	movsw
	xor	al,al			; al = 0 (for return)
	jmps	bdosret
	
;---------------------------------------------------------------------
; bios trap
;---------------------------------------------------------------------
	
biostrap:
;
	sti				; re-enable interrupts
	mov	byte ptr .bioscode,al
	mov	word ptr .bioscx,cx
	mov	word ptr .biosdx,dx
	mov	dx,offset bioscode
	mov	cl,s_bios
	cmp	al,07h
	jb	biostrap_int		; allow non disk bios calls
	cmp	al,0eh
	ja	biostrap_int		; allow non disk bios calls

	cmp	byte ptr cpmver,020h	; cpm-86 version 2.x ?
	jz	biostrap_select		; yes - allow disk bios call

	mov	byte ptr .statcode,01h	; indicate bios call not allowed
	jmps	biostrap_iret		; 8086 status flag set, so return

biostrap_select:
;
	cmp	al,09h			; disk select ?
	jz	biostrap_seldsk

biostrap_int:
;
	push	bp
	push	es
	int	cpm
	pop	es
	pop	bp

biostrap_iret:
;
	iret				; ret far from interrupt

biostrap_seldsk:
;
	push	ds
	push	bp
	push	es

	int	cpm

	or	bx,bx			; invalid disk select ?
	jnz	biostrap_seldsk1

	pop	es
	pop	bp
	pop	ds
	jmps	biostrap_iret		; return

biostrap_seldsk1:
;
	mov	ax,es
	mov	ds,ax			; ds = dph + dpb source seg
	pop	es			; es = dph + dpb dest seg

	mov	si,bx			; si = dph source offset
	mov	di,disk_parm_head	; di = dph dest offset
	mov	cx,08h
	rep	movsw			; copy dph to 8080 dph

	mov	si,0ah[bx]		; si = dpb source offset
	mov	di,disk_parm_block	; di = dpb dest offset
	mov	cx,0fh
	rep	movsb			; copy dpb to 8080 dpb

	mov	bx,disk_parm_head	; bx = dph offset for return (in hl)

	pop	bp			; restore 8080 stack pointer
	pop	ds

	mov	0ah[bx],disk_parm_block	; amend dph to point to 8080 dpb

	jmps	biostrap_iret		; return

;---------------------------------------------------------------------
; error exit - print string then exit
;---------------------------------------------------------------------

error_exit:
;
	call	print_string
	xor	cl,cl
	xor	dl,dl
	jmp	bdos

;---------------------------------------------------------------------
; print string
;---------------------------------------------------------------------

print_string:
;
	mov	cl,c_writestr
	;jmp	bdos

;---------------------------------------------------------------------
; bdos interrupt, es is preserved
;---------------------------------------------------------------------

bdos:
;
	push	es
	int	cpm
	pop	es
	ret

;---------------------------------------------------------------------
; search through leading blanks
;---------------------------------------------------------------------

lead_blank:
;
	cmp	byte ptr 0[bx],0	; end of comline?
	jz	lead_blank1
	cmp	byte ptr 0[bx],' '	; leading blank?
	jnz	lead_blank2
	inc	bx
	jmps	lead_blank

lead_blank1:
	xor	al,al			; set zf flag
	ret

lead_blank2:
	or	al,0ffh			; reset zf flag
	ret

;---------------------------------------------------------------------
