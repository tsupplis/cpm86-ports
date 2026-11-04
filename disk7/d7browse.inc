CMDLOOP:MOV	BX,(Offset RING)	;set start point of listing
	MOV	Word Ptr RINGPOS,BX
DLOOP:	CALL	ILPRT
	DB	CR,LF,'   ',0
LOOP2:	MOV	BX,Word Ptr RINGPOS	;ring filename location
	MOV	AL,M			;move 'fcb' to a-reg and..
	ADD	AL,'A'-1		;..make drive printable (a - p).
	CALL	TYPECHR
	MOV	AL,Byte Ptr C@U@A	;get current (last requested) user area
	OR	AL,AL			;branch if 'user..
	JZ	UAZ			;..area zero'.
	CMP	AL,10			;less then ten?
	JB	LT@TEN			;if yes, branch.
	SUB	AL,10			;if not, suppress leading 10's digit.
	LAHF
	XCHG	AL,AH
	PUSH	AX
	MOV	AL,'1'			;print 10's digit as 'one'
	CALL	TYPECHR
	POP	AX
	XCHG	AL,AH
LT@TEN:	ADD	AL,'0'			;make 1's digit printable
	CALL	TYPECHR
UAZ:	CALL	ILPRT			;fence between 'drive/user' and..
	DB	': ',0			;..'fn.ft'.
	LAHF				;beginning of 'fn.ft' string
	INC	BX
	SAHF
	MOV	CH,8			; 8 filename characters
PRT@FN:	MOV	AL,M
	CALL	TYPECHR
	LAHF
	INC	BX
	SAHF
	DEC	CH
	JNZ	PRT@FN
	MOV	AL,'.'			;period between 'fn' and 'ft'
	CALL	TYPECHR
	MOV	CH,3			; 3 filetype characters
PRT@FT:	MOV	AL,M
	CALL	TYPECHR
	LAHF
	INC	BX
	SAHF
	DEC	CH
	JNZ	PRT@FT
	MOV	AL,M			;get tag (*) and..
	MOV	Byte Ptr TAG+2,AL	;..put after colon.
	INC BX
	MOV AL,M
	PUSH AX
	INC BX
	MOV	Word Ptr RINGPOS,BX	;end of complete record
	MOV AL,' '
	CALL TYPECHR
	POP AX
	PUSH AX
	TEST AL,4
	MOV AL,' '
	JZ HB_PRINT_SYS
	MOV AL,'S'
HB_PRINT_SYS:
	CALL TYPECHR
	POP AX
	TEST AL,2
	MOV AL,' '
	JZ HB_PRINT_HIDDEN
	MOV AL,'H'
HB_PRINT_HIDDEN:
	CALL TYPECHR
	CALL	ILPRT
TAG	DB	' : ',0			;space, colon, space or * before cursor.
	MOV	AL,Byte Ptr J@FLG	;jump..
	OR	AL,AL			;..forward?
	JNZ	L_9	
	JMP	PRE@FOR
L_9:
K@WAIT:	CALL	KEYIN			;wait for character from keyboard
	CALL	CTPROC			;match input command with table.  if no..
	CALL	HELP			;..match, get help message (menu) and..
	CALL	FRESTOR			;..show free storage remaining.
NEUTRAL:MOV	BX,Word Ptr RINGPOS	;stay..
	MOV	DX,-HB_STRIDE			;..in..
	ADD	BX,DX			;..the..
	MOV	Word Ptr RINGPOS,BX	;..same..
	JMP	DLOOP			;..position.
; command-table processor
CTPROC:	MOV	BX,(Offset CMD@TBL)	;point to command table
	MOV	CH,AL
CTPR1:	MOV	AL,M			;get table character and..
	OR	AL,AL
	JNZ	L_10	
	RET				;..return if at table end.
L_10:
	CMP	AL,CH
	JZ	CTPR2			;branch on command character match..
	INC	BX
	INC	BX
	INC	BX
	JMPS	CTPR1			;..else try again.
CTPR2:	LAHF				;point to command routine address and..
	INC	BX
	SAHF
	MOV	AL,M
	LAHF
	INC	BX
	SAHF
	MOV	BH,M
	MOV	BL,AL
	MOV	BP,SP
	XCHG	BX,[BP]
	RET				;..'jump' to it.
; u n t a g
UNTAG:	XOR	AL,AL			;set tag/untag..
	MOV	Byte Ptr T@UN@FG,AL	;..flag to untag.
	MOV	BX,Word Ptr RINGPOS	;move back one..
	MOV	DX,-2			;tag precedes the attribute byte
	ADD	BX,DX			;..and check tagging status.
	MOV	AL,M			;if file previously tagged, remove..
	CMP	AL,'*'			;..size from..
	MOV	M,' '			; (untag character, to next ring position.)
	JZ	FS2			;..summation.
	JMP	FORWARD
; t a g
TAG@EM:	MOV	BX,Word Ptr RINGPOS
	MOV	DX,-2			;tag precedes the attribute byte
	ADD	BX,DX			;..position..
	MOV	AL,M			; (if file
	CMP	AL,'*'			; already tagged, skip
	JNZ	L_11	
	JMP	FORWARD			; to next file.)
L_11:
	MOV	M,'*'			;..and store a '*' tag character.
	MOV	AL,TRUE			;set..
	MOV	Byte Ptr T@UN@FG,AL	;..tag/untag and..
	MOV	Byte Ptr FS@FLG,AL	;..file size flags to tag.
	JMPS	FS2			;get file size
; f i l e   s i z e
; determine and display file size in kilobytes -- round up to next disk
; allocation block -- accumulate tagged file summation
FIL@SIZ:XOR	AL,AL			;set file size/tagged..
	MOV	Byte Ptr FS@FLG,AL	;..file flag to file size.
FS2:	MOV	AL,BS			;backspace over..
	CALL	TYPECHR			;..command character.
	CALL	RINGFCB			;move name to 's$fcb'
; determine file record count and save in 'rcnt'
	MOV	CL,COMPSZ
	MOV	DX,(Offset S@FCB)
	INT	224
	MOV	BX,Word Ptr S@FCB+33
	MOV	Word Ptr RCNT,BX	;save record count and..
	MOV	BX,0
	MOV	Word Ptr S@FCB+33,BX	;..reset cp/m.
; round up to next disk allocation block
	MOV	AL,Byte Ptr B@MASK	;sectors/block - 1
	LAHF				;save 'blm'
	XCHG	AL,AH
	PUSH	AX
	XCHG	AL,AH
	MOV	BL,AL
	XCHG	BX,DX
	MOV	BX,Word Ptr RCNT	;..use here.
	LAHF				;round up to next block
	ADD	BX,DX
	RCR	SI,1
	SAHF
	RCL	SI,1
	MOV	CH,3+1			;convert from..
	CALL	SHIFTLP			;..records to kilobytes.
	POP	AX			;retrieve 'blm'
	XCHG	AL,AH
	ROR	AL,1			;convert..
	ROR	AL,1			;..to..
	ROR	AL,1			;..kilobytes/block.
	AND	AL,1FH
	NOT	AL			;finish rounding
	AND	AL,BL
	MOV	BL,AL			;hl-pair contains # of kilobytes
	MOV	AL,Byte Ptr FS@FLG
	OR	AL,AL
	JZ	D@F@SIZ			;branch if 'f' function
; tagged file size summation
	XCHG	BX,DX			;file size to de-pair
	MOV	AL,Byte Ptr T@UN@FG
	OR	AL,AL
	JZ	TAKE			;if untag, take size from total.
	MOV	BX,Word Ptr TAG@TOT	;accumulate..
	LAHF				;..sum of..
	ADD	BX,DX
	RCR	SI,1
	SAHF
	RCL	SI,1
	MOV	Word Ptr TAG@TOT,BX	;..tagged file sizes.
	XCHG	BX,DX			;file size to hl-pair
	JMPS	D@F@SIZ			;branch to display sizes
TAKE:	MOV	BX,Word Ptr TAG@TOT	;subtract..
	MOV	AL,BL			;..file..
	SUB	AL,DL			;..size..
	MOV	BL,AL			;..from..
	MOV	AL,BH			;..summation..
	SBB	AL,DH			;..total.
	MOV	BH,AL			;then put..
	MOV	Word Ptr TAG@TOT,BX	; (save total)
	XCHG	BX,DX			;..file size in hl-pair.
; display file size in kilobytes -- right justify tagged file total
D@F@SIZ:CALL	DET@BCD			;determine # of bcd digits in hl-pair
	MOV	AL,9			;limit of right margin (good for max cp/m 2.2)
	SUB	AL,CH			; # of digits returned in b-reg from 'det$bcd'
	MOV	Byte Ptr TEST@RT,AL	;save intermediate right-justify data
	CALL	DECOUT			;print individual file size
	CALL	ILPRT
	DB	'k',0
	MOV	AL,Byte Ptr FS@FLG
	OR	AL,AL
	JZ	FORWARD			;show next file if not tagging
; determine # of digits in tagged summation
	MOV	BX,Word Ptr TAG@TOT	;get present summation
	CALL	DET@BCD
; insert necessary spaces (blanks) to right justify display
	MOV	AL,Byte Ptr TEST@RT	;get intermediate right-justify data
	SUB	AL,CH
	MOV	CH,AL
	MOV	AL,' '			;adjust..
ADD@SP:	CALL	TYPECHR			;..to achieve..
	DEC	CH
	JNZ	ADD@SP			;..right justification.
	MOV	AL,'('
	CALL	TYPECHR
	CALL	DECOUT			;print tagged file summation
	CALL	ILPRT
	DB	'k)',0			;to next file..
	JMPS	FORWARD			;..cursor line.
; jump forward 22 files
PRE@FOR:MOV	AL,Byte Ptr J@CNT	;adjust jump..
	INC	AL			;..counter..
	MOV	Byte Ptr J@CNT,AL	;..until..
	CMP	AL,22			;..at top limit.
	JNZ	FORWARD
	MOV	AL,TRUE			;at top, so..
	MOV	Byte Ptr J@FLG,AL	;..turn off jump switch and..
	JMP	K@WAIT			;..wait for next keyboard input.
; j u m p
JUMP22:	XOR	AL,AL			;clear..
	MOV	Byte Ptr J@FLG,AL	;..jump forward flag and..
	MOV	Byte Ptr J@CNT,AL	;..file counter.  fall-thru to next filename.
; f o r w a r d
FORWARD:MOV	BX,Word Ptr RINGPOS	;at end of loop yet?
	XCHG	BX,DX
	MOV	BX,Word Ptr RINGEND
	CALL	CMPDEHL			;compare 'present' to 'end'
	JZ	L_12	
	JMP	DLOOP			;to next print position
L_12:
	CALL	CRLF			;end-of-directory shows with fresh line
	MOV	BX,(Offset RING)	;set position pointer to beginning and..
	MOV	Word Ptr RINGPOS,BX
	JMP	DLOOP			;..redisplay start entry.
; r e v e r s e
REVERSE:MOV	BX,Word Ptr RINGPOS	;see if at beginning of ring
	MOV	DX,(Offset RING)+HB_STRIDE
	CALL	CMPDEHL
	JNZ	REV1			;skip position pointer reset if not..
	CALL	CRLF			;..at beginning.  skip line at junction.
	MOV	BX,Word Ptr RINGEND	;set to end +1 to backup to end
	MOV	DX,HB_STRIDE
	ADD	BX,DX
	MOV	Word Ptr RINGPOS,BX
REV1:	CALL	ILPRT			;indicate reverse
	DB	CR,LF,'<- ',0
	MOV	BX,Word Ptr RINGPOS
	MOV	DX,-(HB_STRIDE*2)		;one ring position..
	ADD	BX,DX			;..backwards.
	MOV	Word Ptr RINGPOS,BX
	JMP	LOOP2			;display without 'crlf'
; s t a t
; determine remaining storage on requested drive
R@DR@ST:CALL	ILPRT
	DB	'torage free on drive: ',0
	CALL	DEF@D@U			;determine drive requested and..
	CALL	RESET			;..login as current.
	CALL	ILPRT
	DB	CR,LF,LF,0
	CALL	FRESTOR			;determine free space remaining
	MOV	AL,Byte Ptr C@DR	;login original as..
	CALL	SET@DR			;..current drive.
	JMP	NEUTRAL
; d e l e t e
; set up to delete filename at cursor position
