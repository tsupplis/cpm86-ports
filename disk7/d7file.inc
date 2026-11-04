DELETE:	CALL	RINGFCB			;move name from ring to 'rename fcb'
	CALL	ILPRT
	DB	'elete? (y/n): ',0
	CALL	KEYIN
	CMP	AL,'Y'
	JZ	L_13	
	JMP	NEUTRAL
L_13:
; delete file
	MOV	DX,(Offset S@FCB)	;point at delete 'fcb'
	MOV	CL,ERASE		;erase function
	INT	224
	INC	AL
	JNZ	DEL2			;file deleted okay
FNF@MSG:CALL	ILPRT			;show error message
	DB	CR,LF,'++ NO FILE FOUND ++',0
	JMP	NEUTRAL
; reverse ring to close up erased position
DEL2:	MOV	BX,Word Ptr RINGPOS	;prepare move up pointers
	PUSH	BX
	MOV	DX,-HB_STRIDE
	LAHF
	ADD	BX,DX
	RCR	SI,1
	SAHF
	RCL	SI,1
	MOV	Word Ptr RINGPOS,BX	;reset current position for move
	XCHG	BX,DX			;de-pair = 'to' location
	POP	BX			;hl-pair = 'from' location
MOVUP:	XCHG	BX,DX
	PUSH	BX			;check if at end
	MOV	BX,Word Ptr RINGEND	;get old end pointer
	CALL	CMPDEHL			;check against current end location
	POP	BX
	XCHG	BX,DX
	JZ	MOVDONE			;must be at end of ring
	MOV	CH,HB_STRIDE			;one name size
	CALL	MOVE			;move one name up
	JMPS	MOVUP			;go check end parameters
MOVDONE:XCHG	BX,DX
	MOV	Word Ptr RINGEND,BX	;set new ring end if all moved
	MOV	DX,(Offset RING)	;see if ring is empty..
	CALL	CMPDEHL			;..(listend --> listpos --> ring)
	JZ	L_14	
	JMP	FORWARD
L_14:
	MOV	BX,Word Ptr RINGPOS
	CALL	CMPDEHL
	JZ	L_15	
	JMP	FORWARD			;neither equal so not empty
L_15:
	MOV	Byte Ptr CANFLG,AL	;zero cancel flag for correct 'log' exit
	CALL	ILPRT
	DB	CR,LF,LF,'    ++ List Empty ++',0
	JMP	TO@MCL			;ask to 'log' or 'exit' (mini-cmdline)
; r e n a m e
; set-up to rename file at cursor position -- scan keyboard buffer and
; move filename to 'rename' destination 'fcb' (dfcb)
RENAME:	MOV	BX,Word Ptr RINGPOS	;move name from ring to rename 'fcb'
	MOV	DX,-HB_STRIDE
	LAHF
	ADD	BX,DX
	RCR	SI,1
	SAHF
	RCL	SI,1
	MOV	DX,(Offset D@FCB)	;place to move name
	MOV	CH,12			;amount to move
	CALL	MOVE
	CALL	ILPRT			;new name prompt
	DB	'ename file to: ',0
	MOV	DX,(Offset D@FCB)+16	;pt to 'fcb' to fill
	CALL	GET@FN			;get filename
	MOV	BX,(Offset D@FCB)+17	;check for wildcards -- none permitted
	MOV	CH,11			; 11 bytes
WILDCHK:MOV	AL,M			;get char
	INC	BX			;pt to next
	CMP	AL,'?'			;wild?
	JZ	WILDFND
	DEC	CH
	JNZ	WILDCHK
; copy old file status bit ($r/o or $sys) to new filename
CPYBITS:MOV	DX,(Offset D@FCB)+1	;first character of old..
	MOV	BX,(Offset D@FCB)+17	;..and of new name.
	MOV	CH,11			; # of bytes with tag bits
CBITS1:	MOV	SI,DX			;fetch bit of old name character
	MOV	AL,[SI]
	AND	AL,80H			;strip upper bit and..
	MOV	CL,AL			;..save in c-reg.
	MOV	AL,7FH			;mask for character only
	AND	AL,M			;put masked character into a-reg
	OR	AL,CL			;add old bit
	MOV	M,AL			;copy new byte back
	LAHF				;bump copy pointers
	INC	BX
	SAHF
	LAHF
	INC	DX
	SAHF
	DEC	CH
	JNZ	CBITS1
; check if new filename already exists; if so, say so.	then go
; to command loop without moving ring position.
	MOV	AL,Byte Ptr D@FCB	;copy drive designator
	MOV	Byte Ptr S@FCB,AL
	MOV	CH,11
	MOV	BX,(Offset D@FCB)+17	;copy new name to..
	MOV	DX,(Offset S@FCB)+1	;..source 'fcb' for existence check.
	CALL	MOVE
	MOV	BX,(Offset S@FCB)+12	;clear cp/m 'fcb' system..
	CALL	INITFCB			;..fields.
	MOV	DX,(Offset S@FCB)	;search to see if this file exists
	MOV	CL,SRCHF		; 'search first' function
	INT	224
	INC	AL			; 0ffh --> 00h if file not found
	JZ	RENFILE			;rename, if duplicate doesn't exists.
	CALL	ILPRT			;announce the situation
	DB	CR,LF,'++ FILE ALREADY EXISTS ++',BELL,0
	JMP	NEUTRAL			;try again?
; wildcard found in filename -- show error
WILDFND:CALL	ILPRT
	DB	CR,LF,'++ NO WILDCARDS ++',BELL,0
	JMP	NEUTRAL
; copy new name into ring position
RENFILE:MOV	BX,Word Ptr RINGPOS	;get ring position pointer
	MOV	DX,-(HB_STRIDE-1)	;leave drive intact; do not overwrite flags
	ADD	BX,DX
	XCHG	BX,DX
	MOV	BX,(Offset D@FCB)+17	;point at new name and..
	MOV	CH,11
	CALL	MOVE			;..move.
	MOV	DX,(Offset D@FCB)	;rename 'fcb' location
	MOV	CL,REN			;rename function
	INT	224
	INC	AL			; 0ffh --> 00h if rename error
	JZ	L_16	
	JMP	NEUTRAL			;if okay, proceed, else..
L_16:
	JMP	FNF@MSG			;..show no-file msg.
; get file name from user and process into fcb pted to by de
GET@FN:	PUSH	DX			;save ptr
	MOV	DX,(Offset CMDBUF)	;command line location
	MOV	CL,RDBUF		;console read-buffer function
	INT	224
	CALL	CONVERT			;capitalize alpha
	POP	BX			;set to null drive
	MOV	M,0			;..required by 'bdos'.
	LAHF
	INC	BX
	SAHF
; initialize new filename field with spaces
	PUSH	BX			;save start pointer
	MOV	CH,11			; # of spaces to 'blank'
	CALL	FILL
	POP	BX
	XCHG	BX,DX
	MOV	BX,(Offset CMDBUF)+1	;put length..
	MOV	CL,M			;..in c-reg.
	INC	BX
	XCHG	BX,DX			;de-pair --> buffer pointer and hl-pair..
	CALL	UNSPACE			;..--> 'fcb' pointer.  remove leading spaces.
; extend buffer to spaces beyond command length
EXTEND:	PUSH	BX
	MOV	BL,CL			;double-byte remaining length
	MOV	BH,0
	ADD	BX,DX			;to buffer end +1
	MOV	M,' '			;force illegal character end
	POP	BX
; start filename scan
SCAN:	MOV	CH,8			;8 characters in filename
SCAN1:	CALL	CKLEGAL			;get and see if legal character
	JNB	L_17	
	JMP	COMCAN			;all of command line?
L_17:
	CMP	AL,' '			;see if end of parameter field
	JNZ	L_18	
	RET				;rename file
L_18:
	CMP	AL,'.'			;at end of filename
	JZ	SCAN2			;process filetype field
	CMP	AL,'*'			;rest wild?
	JZ	SCAN1B
	MOV	M,AL			;put character into destination 'fcb'
	LAHF
	INC	BX
	SAHF
	DEC	CH
	JNZ	SCAN1
; entry if eight characters without a 'period'
SCAN1A:	CALL	CKLEGAL			;scan buffer up to period or end
	JNB	L_19	
	RET				;no extent if not legal
L_19:
	CMP	AL,' '			;end of parameter field?
	JNZ	L_20
	RET
L_20:
	CMP	AL,'.'
	JNZ	SCAN1A			;do till end or period
	JMPS	SCAN2A			;continue at correct place
; make rest of entry wild
SCAN1B:	MOV	M,'?'			;fill with ?'s
	INC	BX
	DEC	CH
	JNZ	SCAN1B
	MOV	SI,DX			;get next char
	MOV	AL,[SI]
	INC	DX			;pt to after dot
	CMP	AL,'.'			;must be dot
	JZ	L_21	
	JMP	COMCAN			;cancel if not
L_21:
	JMPS	SCAN2A
; build filetype field
SCAN2:	LAHF				;advance ptr to file type field
	INC	BX
	SAHF
	DEC	CH
	JNZ	SCAN2
SCAN2A:	MOV	CH,3			;length of filetype field
SCAN3:	CALL	CKLEGAL			;get and check character
	JB	SCAN4			;name done if illegal
	CMP	AL,' '			;end of parameter field?
	JZ	SCAN4
	CMP	AL,'.'			;check if another period
	JZ	SCAN4
	CMP	AL,'*'			;rest wild?
	JZ	SCAN4B
	MOV	M,AL
	LAHF
	INC	BX
	SAHF
	DEC	CH
	JNZ	SCAN3			;get next character
	JMPS	SCAN4A
SCAN4:	LAHF				;advance to end of filetype field..
	INC	BX
	SAHF
	DEC	CH
	JNZ	SCAN4
SCAN4A:	JMP	INITFCB			;..and zero counter fields.  return.
SCAN4B:	MOV	M,'?'			;make wild
	LAHF
	INC	BX
	SAHF
	DEC	CH
	JNZ	SCAN4B
	JMPS	SCAN4A			;complete rest
; g o t o   f i l e
GOTO:	CALL	ILPRT
	DB	'oto filename: ',0
	MOV	DX,(Offset D@FCB)	;point to 'fcb'
	CALL	GET@FN			;get filename
	MOV	BX,(Offset RING)	;point to ring beginning and..
	MOV	Word Ptr RINGPOS,BX	;..set position.
GOTO@LP:CALL	GOTOCMP			;compare?
	JNZ	L_22	
	JMP	DLOOP			;found it, we are there.
L_22:
	MOV	BX,Word Ptr RINGPOS	;advance..
	MOV	DX,HB_STRIDE			;..to..
	LAHF				;..next entry.
	ADD	BX,DX
	SAHF
	MOV	Word Ptr RINGPOS,BX	;put new position..
	XCHG	BX,DX			;..in de-pair.
	MOV	BX,Word Ptr RINGEND	;check for list ending
	CALL	CMPDEHL			;compare current position with end of ring
	JNZ	GOTO@LP			;branch if more to process, else..
	MOV	BX,(Offset RING)+HB_STRIDE	;..point to beginning and..
	MOV	Word Ptr RINGPOS,BX	;..show 1st filename after..
	JMP	FNF@MSG			;..showing file-not-found message.
GOTOCMP:MOV	BX,Word Ptr RINGPOS	;pt to current entry
	INC	BX			;pt to first char of file name
	MOV	DX,(Offset D@FCB)+1	;pt to first char of new file
	MOV	CH,11			; 11 bytes
GOTOC1:	MOV	SI,DX			;get char
	MOV	AL,[SI]
	CMP	AL,'?'			;match?
	JZ	GOTOC2
	CMP	AL,M			;match?
	JZ	L_23	
	RET				;no match
L_23:
GOTOC2:	LAHF				;pt to next
	INC	DX
	SAHF
	LAHF
	INC	BX
	SAHF
	DEC	CH
	JNZ	GOTOC1
	RET
; v i e w
; type file to console with pagination set to 'lps' -- single-line scroll
; using <space> bar, <ctrl-c> or <esc> to cancel, other keys page screen.
VIEW:	CALL	CLS
	CALL	ILPRT
	DB	CR,LF
	DB	'<CTRL-C> cancels, <SP> turns up '
	DB	'one line, '
	DB	'other keys page screen',CR,LF,LF,0
	MOV	AL,1			;initialize..
	MOV	Byte Ptr LPSCNT,AL	;..lines-per-screen counter.
	MOV	Byte Ptr VIEWFLG,AL	;'view' paginate if not zero
	MOV	AL,WRCON		;write console out function
	JMP	CURRENT			;to common i/o processing
; p r i n t e r
; send file to logical list device -- any keypress cancels
LSTFILE:CALL	ILPRT
	DB	'rint on LST Device (y/n)? ',0
	CALL	DKEYIN			;get response
	CMP	AL,'Y'
	JZ	L_24	
	JMP	NEUTRAL
L_24:
	CALL	ILPRT
	DB	' Printing... ',0
	MOV	AL,1			;one for..
	MOV	Byte Ptr VIEWFLG,AL	;..output to printer.
	DEC	AL			;zero for..
	MOV	Byte Ptr LPSCNT,AL	;..lines-per-page counter
	MOV	AL,LISTDEV		;out to 'list' device function and fall thru
; output character for console/list processing
CURRENT:MOV	Byte Ptr CON@LST,AL	;save bdos function
; output file to console or printer
	CALL	RINGFCB			;position name to 'fcb'
	MOV	DX,TBUF			;set to use default cp/m dma buffer
	MOV	CL,SETDMA		;address set function
	INT	224
	MOV	BX,(Offset S@FCB)+12	;point to source extent field
	CALL	INITFCB			;fix 'fcb' for use
	MOV	DX,(Offset S@FCB)	;open file for reading
	MOV	CL,OPEN			;file open function code
	INT	224
	INC	AL			; 0ffh --> 00h if open not okay
	JNZ	ZERO@CR			;if not okay, show error message.
	CALL	ILPRT
	DB	CR,LF,'Unable to Open File',0
	JMP	NEUTRAL
ZERO@CR:XOR	AL,AL
	MOV	Byte Ptr S@FCB+32,AL	;zero file 'current record' field
	MOV	Byte Ptr CHARCNT,AL	;zero char count for tabbing
	CALL	PHEAD			;print heading if output to lst device
READMR:	MOV	DX,(Offset S@FCB)	;point at file 'fcb' for reading
	MOV	CL,READ			;record read function
	INT	224
	OR	AL,AL			;check if read okay
	JZ	L_25	
	JMP	NEUTRAL			;eof?
L_25:
	MOV	BX,TBUF			;point at record just read
	MOV	CH,128			;set record character counter to output
READLP:	MOV	AL,M			;get a character
	AND	AL,7FH			;force to 'ascii'
	CMP	AL,EOFCHAR		;see if end-of-file
	JNZ	L_26	
	JMP	NEUTRAL			;back to ring loop if 'eof'
L_26:
	MOV	DL,AL			;put character for 'bdos' call
	PUSH	CX
	PUSH	BX
	PUSH	DX			;(character in e-reg)
	MOV	AL,Byte Ptr CON@LST	;get function for punch/list/console output
	MOV	CL,AL
	MOV	AL,DL			;check char
	CMP	AL,TAB			;tabulate?
	JNZ	NOTAB
	MOV	DL,' '			;space over
TABL:	PUSH	CX			;save key regs
	PUSH	DX
	INT	224
	POP	DX			;get key regs
	POP	CX
	CALL	INCCCNT			;increment char count
	AND	AL,7			;check for done at every 8
	JNZ	TABL
	JMPS	TABDN
NOTAB:	INT	224			;send character
	CALL	INCCCNT			;increment char count
TABDN:	MOV	AL,Byte Ptr VIEWFLG	;if 'view'..
	OR	AL,AL
	POP	DX			;get char in e in case pager is called
	JZ	L_27	
	CALL	PAGER			;..check for 'lf'.
L_27:
	MOV	DL,GET			;get status or char
	MOV	CL,DIRCON		;console status function
	INT	224			;status?
	POP	BX
	POP	CX
	AND	AL,7FH			;if character there, then abort.
	JZ	L_28	
	CALL	CANVIEW			;already got char
L_28:
	INC	BX			;if not, bump buffer pointer.
	DEC	CH
	JZ	L_29	
	JMP	READLP			;no, more in present record.
L_29:
	JMP	READMR			;yes, get next record.
PAGER:	MOV	AL,DL			;(character in e-reg)
	CMP	AL,LF
	JZ	L_30
	RET
L_30:
	XOR	AL,AL			;zero char count
	MOV	Byte Ptr CHARCNT,AL
	MOV	AL,Byte Ptr CON@LST	;printer or console?
	CMP	AL,LISTDEV		;check for printer
	JZ	PAGEP
	MOV	CH,CTPP			;get number of lines of text per screen
	MOV	AL,Byte Ptr LPSCNT	;is counter..
	INC	AL			;..at..
	MOV	Byte Ptr LPSCNT,AL	;..limit..
	CMP	AL,CH			;..of lines-per-screen?
	JNB	L_31	
	RET				;no, return.
L_31:
	XOR	AL,AL			;yes, initialize..
	MOV	Byte Ptr LPSCNT,AL	;..for next screen full.
	CALL	ILPRT
	DB	'  [more...]',CR,0	;show msg line
	CALL	DKEYIN			;wait for keyboard input
	CMP	AL,' '			;see if <space> bar..
	LAHF
	XCHG	AL,AH
	PUSH	AX
	CALL	CLR@L			;clear above msg line
	POP	AX
	XCHG	AL,AH
	SAHF
	JNZ	CANVIEW			;..if not, see if cancel.
	MOV	AL,CTPP-1		;set for single line..
	MOV	Byte Ptr LPSCNT,AL	;..scroll and..
	RET				;..return for one more line.
PAGEP:	MOV	CH,LTPP			;get number of lines-of-text-per-page
	MOV	AL,Byte Ptr LPSCNT	;is counter..
	INC	AL			;..at..
	MOV	Byte Ptr LPSCNT,AL	;..limit..
	CMP	AL,CH			;..of lines-per-page?
	JNB	L_32	
	RET				;no, return.
L_32:
	XOR	AL,AL			;else, zero..
	MOV	Byte Ptr LPSCNT,AL	;..lines-per-page counter.
	MOV	CH,LSPP			;number of lines to skip
	MOV	CL,LISTDEV		;lst: output
PAGELST:CALL	LCRLF			;new line to lst:
	DEC	CH
	JNZ	PAGELST
	JMPS	PHEAD			;print heading, then done.
CANVIEW:CMP	AL,CTRLC		; ^c?
	JNZ	L_33	
	JMP	COMCAN
L_33:
	CMP	AL,ESCCHR		; 'esc'ape?
	JNZ	L_34	
	JMP	COMCAN
L_34:
	RET				;return for another page
INCCCNT:MOV	AL,Byte Ptr CHARCNT	;increment char count
	INC	AL
	MOV	Byte Ptr CHARCNT,AL
	RET
PHEAD:	MOV	AL,Byte Ptr CON@LST	;printing to printer?
	CMP	AL,LISTDEV
	JZ	L_35
	RET
L_35:
	MOV	BX,(Offset HEADMSG)	;print heading
PHEAD1:	MOV	AL,M			;get char
	OR	AL,AL			;done?
	JZ	PHEAD2
	CALL	LOUT			;send to printer
	INC	BX			;pt to next
	JMPS	PHEAD1
PHEAD2:	MOV	BX,(Offset S@FCB)+1	;pt to file name
	MOV	CH,8			; 8 chars
	CALL	PHEAD3
	MOV	AL,'.'			;dot
	CALL	LOUT
	MOV	CH,3			; 3 more chars
	CALL	PHEAD3
	CALL	LCRLF			;new line
	CALL	LCRLF			;blank line
	RET
PHEAD3:	MOV	AL,M			;get char
	CALL	LOUT			; 'lst:' it
	LAHF				;pt to next
	INC	BX
	SAHF
	DEC	CH
	JNZ	PHEAD3
	RET
L_36	EQU	$
	DSEG
	ORG	Offset L_36
HEADMSG	DB	'File: ',0
L_37	EQU	$
	CSEG
	ORG	Offset L_37
; m a s s   c o p y
; copy files tagged using the 't' command.  auto-erase if file exists
; on requested destination drive or in user area.
