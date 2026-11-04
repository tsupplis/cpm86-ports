MASS:	MOV	BX,(Offset RING)+12	;get 1st possible tag location
	MOV	Word Ptr RINGPOS,BX
MASS@LP:MOV	AL,'*'
	CMP	AL,M
	LAHF				;get in filename synchronization
	INC BX
	INC BX
	SAHF
	MOV	Word Ptr RINGPOS,BX
	JZ	MCOPY			;copy filename with tag character (*)
M@LP:	MOV	BX,Word Ptr RINGPOS	;re-entry point for next file mass-copy
	XCHG	BX,DX			;at ring..
	MOV	BX,Word Ptr RINGEND	;..end yet?
	CALL	CMPDEHL			; (compare present position with end)
	MOV	BX,Word Ptr RINGPOS
	JZ HB_MASS_DONE
	ADD BX,12 ; next record tag; flags are not a tag
	JMP MASS@LP
HB_MASS_DONE:
	XOR	AL,AL			;reset flags..
	MOV	Byte Ptr FIRST@M,AL	;..for..
	NOT	AL			;..next..
	MOV	Byte Ptr MFLAG,AL	;..mass-copy request.
	JMP	CMDLOOP			;jump to 'ring' beginning
; c o p y
; copy source file at current 'ring' position to another drive/user area.
; set-up fcb's and buffer area and check for correct keyboard inputs.
; provides automatic 'crc' file copy verification.
MCOPY:	XOR	AL,AL			;zero flag to..
	MOV	Byte Ptr MFLAG,AL	;..mass copy.
COPY:	MOV	BX,0			;initialize storage for..
	MOV	Word Ptr CRCVAL,BX	;..'crc' working value.
	CALL	RINGFCB			;move from 'ring' to 's$fcb'
	MOV	BX,(Offset S@FCB)+12	;init cp/m system..
	CALL	INITFCB			;..fields and..
	XOR	AL,AL			;..the..
	MOV	Byte Ptr S@FCB+32,AL	;..'cr' field.
	MOV	CH,32			;copy source 'fcb' to destination 'fcb'
	MOV	BX,(Offset S@FCB)+1	;from point..
	MOV	DX,(Offset D@FCB)+1	;..to point..
	CALL	MOVE			;..copy across.
	MOV	DX,(Offset S@FCB)	;open file..
	MOV	CL,OPEN			;..for reading.
	INT	224
	INC	AL			; 0ffh --> 00h if bad open
	JNZ	COPY2			;if okay, skip error message.
	CALL	ILPRT
	DB	CR,LF,'Can''t Open Source',BELL,0
	JMP	NEUTRAL
COPY2:	MOV	AL,Byte Ptr FIRST@M	;by-pass prompt, drive/user compatibility..
	OR	AL,AL			;..test, and disk reset after..
	JNZ	COPY3M			;..1st time thru in mass-copy mode.
	CALL	ILPRT			;prompt for drive selection
	DB	BS,'Copy to DIRectory: ',0
	CALL	DEF@D@U
; either drives or user areas must be different
	MOV	AL,Byte Ptr .FCB	;get requested drive from 'fcb' and..
	MOV	CH,AL			;..put into b-reg for..
	MOV	AL,Byte Ptr S@FCB	;..comparison.
	CMP	AL,CH
	JNZ	COPY3			;branch if different
	MOV	AL,Byte Ptr R@U@A	;requested user area --> rua
	MOV	CH,AL
	MOV	AL,Byte Ptr C@U@A	;current user area --> cua
	CMP	AL,CH
	JNZ	COPY3
	CALL	ILPRT			;if not, show error condition:
	DB	CR,LF,BELL
	DB	'++ Drives or User Areas must be '
	DB	'different ++'
	DB	0
	JMP	NEUTRAL			;try again?
COPY3:	CALL	RESET			;make sure disk is read/write
COPY3M:	MOV	AL,Byte Ptr .FCB	;put requested drive into..
	MOV	Byte Ptr D@FCB,AL	;..place in destination fcb.
	MOV	AL,Byte Ptr R@U@A	;toggle to..
	CALL	SET@USR			;..requested user area.
	MOV	AL,Byte Ptr MFLAG	;auto-erase..
	OR	AL,AL			;..if..
	JZ	COPY4M			;..in mass-copy mode.
	MOV	DX,(Offset D@FCB)	;search for duplicate
	MOV	CL,SRCHF		; 'search first' function
	INT	224
	INC	AL			;if not found, 0ffh --> 00h.  then..
	JZ	COPY5			;go to 'make' function for new file.
	CALL	ILPRT			;if found, ask to replace:
	DB	CR,LF
	DB	' ---> Copy exists, erase? (y/n):'
	DB	' '
	DB	0
	CALL	KEYIN			;get answer
	CMP	AL,'Y'			;if yes, then..
	JZ	COPY4M			;..delete and overlay.
	MOV	AL,Byte Ptr C@U@A	;reset to..
	CALL	SET@USR			;..current user area.
	JMP	FORWARD			;if re-copy not wanted, to next position.
COPY4M:	MOV	DX,(Offset D@FCB)	;delete file already existing
	MOV	CL,ERASE		;erase function
	INT	224
COPY5:	MOV	DX,(Offset D@FCB)	;create new file and open for writing
	MOV	CL,MAKE			;make function
	INT	224
	INC	AL			;if directory full, 0ffh --> 00h.
	JNZ	COPY6			;if not, branch.
	CALL	ILPRT
	DB	CR,LF,'Destination Directory Full',BELL,0
	JMP	NEUTRAL			;if error, back to ring processor.
COPY6:	MOV	CH,8			;show filename and..
	MOV	BX,(Offset D@FCB)+1
	MOV	DX,(Offset COPYMFN)
	CALL	MOVE
	INC	DX
	MOV	CH,3			;..filetype during copy.
	CALL	MOVE
	MOV	AL,Byte Ptr FIRST@M	;if 1st time thru mass-copy..
	OR	AL,AL			;..mode, add..
	MOV	AL,LF			;..a line feed.
	JNZ	L_38	
	CALL	TYPECHR
L_38:
	CALL	CLR@L			;clear line
	CALL	ILPRT
	DB	CR,' ---> Copying file '
COPYMFN	DB	'        .    ',0
	XOR	AL,AL			;clear 'eof'..
	MOV	Byte Ptr EOFLAG,AL	;..flag.
COPY6A:	MOV	AL,Byte Ptr C@U@A	;reset user area..
	CALL	SET@USR			;..to current.
	MOV	BX,0			;clear current-record..
	MOV	Word Ptr REC@CNT,BX	;..counter.
	MOV	BX,Word Ptr BUFSTART	;set buffer start pointer..
	MOV	Word Ptr BUF@PT,BX	;..to begin pointer.
; read source file -- fill buffer memory or stop on 'eof' -- update 'crc'
; on-the-fly
COPY7:	MOV	BX,Word Ptr BUF@PT	;set dma address to buffer pointer
	XCHG	BX,DX			; de-pair --> dma address
	MOV	CL,SETDMA
	INT	224
	MOV	DX,(Offset S@FCB)	;source 'fcb' for reading
	MOV	CL,READ			;record read function
	INT	224
	OR	AL,AL			; 00h --> read okay
	JZ	S@RD@OK
	DEC	AL			;eof?
	JZ	COPY8			;yes, end-of-file, set 'eof' flag.
	CALL	ILPRT
	DB	CR,LF,'Source Read Error',BELL,0
	JMP	NEUTRAL
S@RD@OK:MOV	BX,Word Ptr BUF@PT
	MOV	CH,128
COPY7A:	MOV	AL,M			;get character and..
	CALL	UPDCRC			;..add to 'crc' value.
	INC	BX
	DEC	CH
	JNZ	COPY7A			;loop 'till record read finished
	MOV	BX,Word Ptr BUF@PT	;bump buffer pointer..
	MOV	DX,128			;..by..
	LAHF				;..one..
	ADD	BX,DX
	SAHF
	MOV	Word Ptr BUF@PT,BX	;..record.
	MOV	BX,Word Ptr REC@CNT	;bump buffer..
	LAHF				;..record count and..
	INC	BX
	SAHF
	MOV	Word Ptr REC@CNT,BX	;..store.
	XCHG	BX,DX			;ready to compare to..
	MOV	BX,Word Ptr REC@MAX	;..maximum record count (full-buffer).
	CALL	CMPDEHL			;compare
	JNZ	COPY7			;if not full, get next record.
	JMPS	COPY9			;full, start first write session.
; indicate end-of-file read
COPY8:	MOV	AL,TRUE			;set 'eof' flag
	MOV	Byte Ptr EOFLAG,AL
; write 'read-file' from memory buffer to destination 'written-file'
COPY9:	MOV	AL,Byte Ptr R@U@A	;set user to requested..
	CALL	SET@USR			;..area.
	MOV	BX,Word Ptr BUFSTART	;adjust buffer pointer..
	MOV	Word Ptr BUF@PT,BX	;..to start address.
COPY10:	MOV	BX,Word Ptr REC@CNT	;buffer empty?
	MOV	AL,BH
	OR	AL,BL
	JZ	COPY11			;buffer empty, check 'eof' flag.
	DEC	BX			;dec buffer record count for each write
	MOV	Word Ptr REC@CNT,BX
	MOV	BX,Word Ptr BUF@PT	;set up dma address
	PUSH	BX			;save for size bump
	XCHG	BX,DX			;pointer in de-pair
	MOV	CL,SETDMA
	INT	224
	POP	BX
	MOV	DX,128			;bump pointer one record length
	ADD	BX,DX
	MOV	Word Ptr BUF@PT,BX
	MOV	DX,(Offset D@FCB)	;destination file 'fcb'
	MOV	CL,WRITE		;write record function
	INT	224
	OR	AL,AL			; 00h --> write okay
	JZ	COPY10			;okay, do next record.	else..
	CALL	ILPRT			;..say disk write error.
	DB	CR,LF,'++ COPY DISK FULL ++',BELL,0
C@ERA:	MOV	DX,(Offset D@FCB)	;delete..
	MOV	CL,ERASE		;..partial..
	INT	224			;..from directory.
	XOR	AL,AL			;reset 1st-time-thru tag flag..
	MOV	Byte Ptr FIRST@M,AL	;..for continuation of mass copying.
	JMP	NEUTRAL			;back to ring
COPY11:	MOV	AL,Byte Ptr EOFLAG	;buffer all written, check for 'eof'.
	OR	AL,AL
	JNZ	L_39	
	JMP	COPY6A			;branch to read next buffer full
L_39:
	MOV	DX,(Offset D@FCB)	;point at 'fcb' for file closure
	MOV	CL,CLOSE
	INT	224
	INC	AL			;if no-close-error then..
	JNZ	CRC@CMP			;..compare file crc's.
	CALL	ILPRT
	DB	CR,LF,'Copy Close Error',BELL,0
	JMPS	C@ERA
; read destination 'written-file' and compare crc's
CRC@CMP:MOV	BX,Word Ptr CRCVAL	;transfer 'crc' value to..
	MOV	Word Ptr CRCVAL2,BX	;..new storage area.
	MOV	BX,0			;clear working storage..
	MOV	Word Ptr CRCVAL,BX	;..to continue.
	MOV	DX,TBUF
	MOV	CL,SETDMA
	INT	224
	MOV	BX,(Offset D@FCB)+12	;init cp/m..
	CALL	INITFCB			;..system counters.
	MOV	DX,(Offset D@FCB)
	MOV	CL,OPEN
	INT	224
	INC	AL			; 0ffh --> 00h if bad open
	JNZ	L_40	
	JMP	BADCRC			;if bad open, just say 'bad-crc'.
L_40:
	XOR	AL,AL			;zero 'fcb'..
	MOV	Byte Ptr D@FCB+32,AL	;..'cr' field.
CRCWF1:	MOV	DX,(Offset D@FCB)
	MOV	CL,READ
	INT	224
	OR	AL,AL			;read okay?
	JZ	D@RD@OK			;yes, read more.
	DEC	AL			;eof?
	JZ	FINCRC			;yes, finish up and make 'crc' comparison.
	CALL	ILPRT
	DB	CR,LF,'Copy Read Error',BELL,0
	JMP	NEUTRAL
D@RD@OK:MOV	BX,TBUF
	MOV	CH,128
CRCWF2:	MOV	AL,M			;get character to..
	CALL	UPDCRC			;..add to 'crc' value. 
	INC	BX
	DEC	CH
	JNZ	CRCWF2
	JMPS	CRCWF1
; crc subroutines
; update 'crc' word value
UPDCRC:	PUSH	CX			;update 'crc'..
	PUSH	BX			;..accumulator..
	MOV	BX,Word Ptr CRCVAL	;pick up partial remainder
	XCHG	BX,DX			;de-pair now has partial
	MOV	CH,0
	XOR	AL,DH
	MOV	CL,AL
	MOV	BX,(Offset CRCTBL)
	ADD	BX,CX
	MOV	AL,M
	XOR	AL,DL
	MOV	DH,AL
	INC	BH
	MOV	DL,M
	XCHG	BX,DX
	MOV	Word Ptr CRCVAL,BX
	POP	BX
	POP	CX
	RET
; verfiy read source and destination file
FINCRC:	MOV	AL,Byte Ptr C@U@A	;reset user from 'requested'..
	CALL	SET@USR			;..to 'current' area.
	MOV	BX,Word Ptr CRCVAL	;put written-file 'crc' into..
	XCHG	BX,DX			;..de-pair.
	MOV	BX,Word Ptr CRCVAL2	;put read-file 'crc' and..
	CALL	CMPDEHL			;..compare 'de/hl' for equality.
	JNZ	BADCRC			;if not zero, show copy-error message.
	CALL	ILPRT			;if zero, show 'verified' message.
	DB	CR,' ---> File copy certified       ',0
	MOV	AL,Byte Ptr MFLAG	;if not mass-copy mode, return..
	OR	AL,AL			;..to next 'ring' position.
	JZ	L_41	
	JMP	FORWARD			;else..
L_41:
	NOT	AL			;..set 1st-time-thru flag..
	MOV	Byte Ptr FIRST@M,AL	;..and..
	JMP	M@LP			;..get next file to copy, if one.
; copy no-verfiy message
BADCRC:	CALL	ILPRT
	DB	CR,LF,'++ FILE CRC ERROR ++',BELL,0
	JMP	FORWARD			;move to next 'ring' position
; initialize tables for fast crc calculations
INITCRC:MOV	BX,(Offset CRCTBL)
	MOV	CL,0			;table index
GLOOP:	XCHG	BX,DX
	MOV	BX,0			;initialize crc register pair
	MOV	AL,CL
	PUSH	CX			;save index in c-reg
	MOV	CH,8
	XOR	AL,BH
	MOV	BH,AL
LLOOP:	SHL	BX,1
	JNB	LSKIP
	MOV	AL,10H			;generator is x^16 + x^12 + x^5 + x^0 as..
	XOR	AL,BH			;..recommended by ccitt for asynchronous..
	MOV	BH,AL			;..communications.  produces same results..
	MOV	AL,21H			;..as popular programs chek, comm7, mdm7,..
	XOR	AL,BL			;..and modem7.
	MOV	BL,AL
LSKIP:	DEC	CH
	JNZ	LLOOP
	POP	CX
	XCHG	BX,DX			;de-pair has 'crc', hl-pair points to table.
	MOV	M,DH			;store high..
	INC	BH			;..and..
	MOV	M,DL			;..low byte of 'crc'.
	DEC	BH
	LAHF				;move to next table entry
	INC	BX
	SAHF
	INC	CL			;next index
	JNZ	GLOOP
	RET
