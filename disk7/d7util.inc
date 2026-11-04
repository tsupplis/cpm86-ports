; w o r k h o r s e   r o u t i n e s
; inline print of message
ILPRT:	MOV	BP,SP			;save hl, get msg pointer.
	XCHG	BX,[BP]
ILPLP:	MOV	AL,M			;get character
	INC	BX			;pt to next
	AND	AL,7FH			;strip type bits
	JZ	ILPLP1
	CALL	TYPECHR			;show on console
	JMPS	ILPLP
ILPLP1:	MOV	BP,SP			;set hl-pair and..
	XCHG	BX,[BP]
	RET				;..return past message.
; output 'crlf' to console
CRLF:	MOV	AL,CR
	CALL	TYPECHR
	MOV	AL,LF
; conout routine (re-entrant)
TYPECHR:LAHF
	XCHG	AL,AH
	PUSH	AX
	XCHG	AL,AH
	PUSH	CX
	PUSH	DX
	PUSH	BX
	MOV	DL,AL
	MOV	CL,WRCON
	INT	224
	POP	BX
	POP	DX
	POP	CX
	POP	AX
	XCHG	AL,AH
	SAHF
	RET
; crt clear-line function (cursor left at left-side of screen)
CLR@L:	CALL	CRONLY
	MOV	CH,30			; # of characters to blank on line
	MOV	AL,' '
CL@LP:	CALL	TYPECHR
	DEC	CH
	JNZ	CL@LP
; output 'cr' to console
CRONLY:	MOV	AL,CR
	JMPS	TYPECHR
; output 'crlf' to printer
LCRLF:	MOV	AL,CR
	CALL	LOUT
	MOV	AL,LF
; printer routine
LOUT:	LAHF
	XCHG	AL,AH
	PUSH	AX
	XCHG	AL,AH
	PUSH	CX
	PUSH	DX
	PUSH	BX
	MOV	DL,AL
	MOV	CL,LISTDEV
	INT	224
	POP	BX
	POP	DX
	POP	CX
	POP	AX
	XCHG	AL,AH
	SAHF
	RET
; direct 'bios' conin routine (waits for response)
DKEYIN:	CALL	CIN			;get character from 'bios'
	AND	AL,7FH			;mask 'msb'
	JMPS	UCASE			;capitalize
; CP/M-86: no BIOS jump table at 0001h, poll BDOS 6 (direct console input)
CIN:	MOV	CL,6
	MOV	DL,0FFH
	INT	224
	OR	AL,AL
	JZ	CIN
	RET
; conin routine (waits for response)
KEYIN:	MOV	CL,RDCON
	INT	224
; convert character in a-reg to upper case
UCASE:	CMP	AL,61H			;less than small 'a'?
	JNB	L_42	
	RET				;if so, no convert needed.
L_42:
	CMP	AL,7AH+1		;>small 'z'?
	JNAE	L_43	
	RET				;if so, ignore.
L_43:
	AND	AL,5FH			;otherwise convert
	RET
; convert keyboard input to upper case
CONVERT:MOV	BX,(Offset CMDBUF)+1	; 'current keyboard buffer length'..
	MOV	CH,M			;..to b-reg.
	MOV	AL,CH
	OR	AL,AL			;if zero length, skip conversion.
	JNZ	L_44	
	JMP	COMCAN
L_44:
CONVLP:	LAHF				;point at character to capitalize
	INC	BX
	SAHF
	MOV	AL,M
	CALL	UCASE
	MOV	M,AL			;put back into buffer
	DEC	CH
	JNZ	CONVLP
	RET
; fill buffer with 'spaces' with count in b-reg
FILL:	MOV	M,' '			;put in space character
	LAHF
	INC	BX
	SAHF
	DEC	CH
	JNZ	FILL			;no, branch.
	RET
; ignore leading spaces (ls) in buffer, length in c-reg.
UNSPACE:MOV	SI,DX			;get character
	MOV	AL,[SI]
	CMP	AL,' '
	JZ	L_45	
	RET				;not blank, a file is entered.
L_45:
	INC	DX			;to next character
	DEC	CL
	JNZ	L_46	
	JMP	COMCAN			;all spaces --> command recovery error
L_46:
	JMPS	UNSPACE
; check for legal cp/m filename character -- return with carry set if illegal
CKLEGAL:MOV	SI,DX			;get character from de-pair
	MOV	AL,[SI]
	INC	DX			;point at next character
	CMP	AL,' '			;less than space?
	JNB	L_47	
	RET				;return carry if unpermitted character
L_47:
	PUSH	BX
	PUSH	CX
	CMP	AL,'['			;if greater than 'z', exit with..
	JNB	CKERR			;..carry set.
	MOV	CH,(Offset CHR@TEND)-(Offset CHR@TBL)
	MOV	BX,(Offset CHR@TBL)
CHR@LP:	CMP	AL,M
	JZ	CKERR
	INC	BX
	DEC	CH
	JNZ	CHR@LP
	OR	AL,AL			;clear carry for good character
	POP	CX
	POP	BX
	RET
CKERR:	POP	CX
	POP	BX
	STC				;error exit with carry set
	RET
L_48	EQU	$
	DSEG
	ORG	Offset L_48
CHR@TBL	DB	',',':',';','<','=','>'	;invalid character table
L_49	EQU	$
	CSEG
	ORG	Offset L_49
CHR@TEND RS	0
; clear console crt screen
CLS:
	MOV	CH,24			;output lf's (fully clear screen)
	MOV	AL,LF
LFLP:	CALL	TYPECHR
	DEC	CH
	JNZ	LFLP			;bump b-reg down to zero
;not hz19
	RET
; copy filename from 'ring' to 's$fcb'
RINGFCB:MOV	BX,Word Ptr RINGPOS	;copy name from ring to source 'fcb'
	MOV	DX,-HB_STRIDE			;subtract 13 to..
	ADD	BX,DX			;..point to name position.
	MOV	DX,(Offset S@FCB)	;place to copy filename and..
	MOV	CH,12			;..amount to move.
; move subroutine -- copy b-reg # of bytes from hl-pair to de-pair
MOVE:	MOV	AL,M			;get hl-pair referenced source byte
	AND	AL,7FH			;strip cp/m 2.x attributes
	MOV	SI,DX			;put to de-pair referenced destination
	MOV	[SI],AL
	LAHF				;fix pointers..
	INC	BX
	SAHF
	LAHF				;..for next movement.
	INC	DX
	SAHF
	DEC	CH
	JNZ	MOVE			;dec byte count and test if done
	RET
; initialize 'fcb' cp/m system fields (entry with hl-pair pointing to 'fcb')
INITFCB:MOV	CH,4			;fill ex, s1, s2, rc counters with zeros.
INITLP:	MOV	M,0			;put zero (null) in memory
	LAHF
	INC	BX
	SAHF
	DEC	CH
	JNZ	INITLP
	RET
; disk system reset -- login requested drive
RESET:	MOV	CL,INQDISK		;determine and..
	INT	224			;..save..
	MOV	Byte Ptr C@DR,AL	;..current drive.
	MOV	CL,RESETDK		;reset system
	INT	224
	MOV	AL,Byte Ptr R@DR	;login requested..
SET@DR:	MOV	DL,AL			;..drive.
	MOV	CL,LOGIN
	INT	224			;return to caller
	RET
; set/reset (or get) user area (call with binary user area in a-reg)
SET@USR:MOV	DL,AL			; 0 --> 0, 1 --> 1, etc.
GET@USR:MOV	CL,SGUSER
	INT	224			;return to caller
	RET
; compare de-pair to hl-pair and set flags accordingly
CMPDEHL:MOV	AL,DH			;see if high bytes set flags
	CMP	AL,BH
	JZ	L_50	
	RET				;return if not equal
L_50:
	MOV	AL,DL
	CMP	AL,BL			;low bytes set flags instead
	RET
; shift hl-pair b-reg bits (-1) to right (divider routine)
SHIFTLP:DEC	CH
	JNZ	L_51
	RET
L_51:
	MOV	AL,BH
	OR	AL,AL
	RCR	AL,1
	MOV	BH,AL
	MOV	AL,BL
	RCR	AL,1
	MOV	BL,AL
	JMPS	SHIFTLP
; decimal pretty print (h-reg contains msb; l-reg, the lsb.)
DECOUT:	LAHF
	XCHG	AL,AH
	PUSH	AX
	PUSH	CX
	PUSH	DX
	PUSH	BX
	MOV	CX,-10			;radix
	MOV	DX,-1
DECOU2:	ADD	BX,CX			;sets..
	INC	DX
	JB	DECOU2			;..carry.	
	MOV	CX,10
	ADD	BX,CX
	XCHG	BX,DX
	MOV	AL,BH
	OR	AL,BL
	JZ	L_52	
	CALL	DECOUT			; (recursive)
L_52:
	MOV	AL,DL
	ADD	AL,'0'			;make ascii
	CALL	TYPECHR
	POP	BX
	POP	DX
	POP	CX
	POP	AX
	XCHG	AL,AH
	SAHF
	RET
; determine # of bcd digits in hl-pair -- place # in b-reg
DET@BCD:MOV	DX,9			;test for less than 10
	CALL	CMPDEHL			;compare and..
	MOV	CH,1			; (one bcd digit)
	JNAE	L_53	
	RET				;..return if not carry.
L_53:
	MOV	DL,99			;less than 100?
	CALL	CMPDEHL
	MOV	CH,2
	JNAE	L_54
	RET
L_54:
	MOV	DX,999			; <1000?
	CALL	CMPDEHL
	MOV	CH,3
	JNAE	L_55
	RET
L_55:
	MOV	CH,4			;assume >999  (4 digits)
	RET
; determine free storage remaining on selected drive
FRESTOR:MOV	CL,INQDISK		;determine current drive
	INT	224			;returns 0 as a:, 1 as b:, etc.
	INC	AL			;make 1 --> a:, 2 --> b:, etc.
	MOV	Byte Ptr .FCB,AL
	ADD	AL,'A'-1		;make printable and..
	MOV	Byte Ptr DRNAME,AL	;..use as drive designator.
	MOV	CL,GETPARM		;current disk parameter block
	INT	224			;CP/M-86: returned in ES:BX
	INC	BX			;bump to..
	INC	BX
	MOV	AL,ES:M			;..block shift factor.
	MOV	Byte Ptr BSHIFTF,AL	; 'bsh'
	INC	BX			;bump to..
	MOV	AL,ES:M			;..block mask.
	MOV	Byte Ptr B@MASK,AL	; 'blm'
	INC	BX			;bump to..
	INC	BX			;..get..
	MOV	DL,ES:M			;..maximum block number..
	INC	BX			;..double..
	MOV	DH,ES:M			;..byte.
	XCHG	BX,DX
	MOV	Word Ptr B@MAX,BX	; 'dsm'
	MOV	CL,GETVERS		;get cp/m version in hl-pair
	INT	224
	MOV	AL,BL			;get version #
	CMP	AL,30H			; 3.x?
	JB	FREE20			;use old method if not
	MOV	AL,Byte Ptr .FCB	;get drive #
	DEC	AL
	MOV	DL,AL			;use new compute free space bdos call
	MOV	CL,FRESPC
	INT	224
	MOV	CL,3			;answer is a 24-bit integer
FRE3L1:	MOV	BX,CPM@BASE+82H		;answer is in 1st 3 bytes of 'dma' address
	MOV	CH,3			;convert from sectors to kilobytes..
	OR	AL,AL			;..by dividing by 8.
FRE3L2:	MOV	AL,M
	RCR	AL,1
	MOV	M,AL
	LAHF
	DEC	BX
	SAHF
	DEC	CH
	JNZ	FRE3L2			;loop for 3 bytes
	DEC	CL
	JNZ	FRE3L1			;shift 3 times
	MOV	BX,Word Ptr .CPM@BASE+80H	;now get result in k
	JMPS	PRT@FRE			;go store it
FREE20:	MOV	CL,INQALC		;address of cp/m allocation vector
	INT	224			;CP/M-86: returned in ES:BX
	XCHG	BX,DX			;get its length
	MOV	BX,Word Ptr B@MAX
	INC	BX
	MOV	CX,0			;initialize block count to zero
GSPBYT:	PUSH	DX			;save allocation address
	MOV	SI,DX
	MOV	AL,ES:[SI]
	MOV	DL,8			;set to process 8 bits (blocks)
GSPLUP:	RCL	AL,1			;test bit
	JB	NOT@FRE
	INC	CX
NOT@FRE:MOV	DH,AL			;save bits
	DEC	BX
	MOV	AL,BL
	OR	AL,BH
	JZ	END@ALC			;quit if out of blocks
	MOV	AL,DH			;restore bits
	DEC	DL			;count down 8 bits
	JNZ	GSPLUP			;branch to do another bit
	POP	DX			;bump to next count..
	INC	DX			;..of allocation vector.
	JMPS	GSPBYT			;process it
END@ALC:POP	DX			;clear alloc vector pointer from stack
	MOV	BL,CL			;copy # blocks to hl-pair
	MOV	BH,CH
	MOV	AL,Byte Ptr BSHIFTF	;get block shift factor
	SUB	AL,3			;convert from sectors to thousands (k)
	JZ	PRT@FRE			;skip shifts if 1k blocks
FREK@LP:SHL	BX,1			;multiply blocks by k-bytes per block
	DEC	AL			;multiply by 2, 4, 8, or 16.
	JNZ	FREK@LP
PRT@FRE:CALL	DECOUT			; # of free k-bytes in hl-pair
	CALL	ILPRT
	DB	'k bytes free on DIRectory '
DRNAME	DB	' :',CR,LF,'   ',0
	RET
; HIDDEN is an in-memory flag, never a CP/M filename attribute.
; Raw search with user '?' yields CDOS FAT's synthetic header. Version is
; only an initial gate: allocation bytes are decoded ONLY after that marker.
; No BDOS 230, DOS interrupt, sibling library or on-disk change is needed.
HB_PROBE:
 PUSH AX
 PUSH BX
 PUSH CX
 PUSH DX
 MOV Byte Ptr HB_FAT,0
 MOV CL,12
 INT 224
 CMP AL,41H
 JB HB_PROBE_DONE
 MOV CL,25
 INT 224
 MOV Byte Ptr HB_DRIVE,AL
 MOV DL,Byte Ptr .FCB
 OR DL,DL
 JZ HB_PROBE_SEARCH
 DEC DL
 MOV CL,14
 INT 224
HB_PROBE_SEARCH:
 MOV DX,Offset HB_RAW
 MOV CL,17
 INT 224
 CMP AL,0FFH
 JE HB_PROBE_RESTORE
 AND AX,3
 MOV CL,5
 SHL AX,CL
 MOV BX,AX
 ADD BX,TBUF
 CMP Byte Ptr [BX],20H
 JNE HB_PROBE_RESTORE
 CMP Byte Ptr 15[BX],80H
 JNE HB_PROBE_RESTORE
 CMP Word Ptr 16[BX],8080H
 JNE HB_PROBE_RESTORE
 MOV Byte Ptr HB_FAT,1
HB_PROBE_RESTORE:
 MOV DL,Byte Ptr HB_DRIVE
 MOV CL,14
 INT 224
HB_PROBE_DONE:
 POP DX
 POP CX
 POP BX
 POP AX
 RET
; BX is the found directory entry. Return carry if excluded. Preserve BX.
; Bit 0=RO, bit 1=FAT HIDDEN, bit 2=SYSTEM; low bits survive MOVBYT.
HB_FLAGS:
 XOR AL,AL
 TEST Byte Ptr 9[BX],80H
 JZ HB_FLAGS_SYS
 OR AL,1
HB_FLAGS_SYS:
 TEST Byte Ptr 10[BX],80H
 JZ HB_FLAGS_HIDDEN
 OR AL,4
HB_FLAGS_HIDDEN:
 CMP Byte Ptr HB_FAT,0
 JE HB_FLAGS_SELECT
 MOV AH,Byte Ptr 16[BX]
 CMP AH,Byte Ptr 17[BX]
 JNE HB_FLAGS_SELECT
 AND AH,0A0H
 CMP AH,0A0H
 JNE HB_FLAGS_SELECT
 OR AL,2
HB_FLAGS_SELECT:
 MOV Byte Ptr HB_LAST,AL
 CMP Byte Ptr HB_ALL,0
 JNE HB_FLAGS_KEEP
 TEST AL,6
 JZ HB_FLAGS_KEEP
 STC
 RET
HB_FLAGS_KEEP:
 CLC
 RET
HB_TOGGLE:
 XOR Byte Ptr HB_ALL,1
 JMP PLUNGE
HB_EMPTY:
 CALL ILPRT
 DB CR,LF,'No matching normal files. O toggles all; X exits: ',0
 CALL KEYIN
 CMP AL,'O'
 JNE HB_EMPTY_EXIT
 JMP HB_TOGGLE
HB_EMPTY_EXIT:
 JMP CPM@CCP
L_56	EQU	$
	DSEG
	ORG	Offset L_56
; s t o r a g e
; initialized
; command table
; Persistent browse options and metadata, outside the movable list.
HB_ALL DB 0
HB_FAT DB 0
HB_LAST DB 0
HB_DRIVE DB 0
HB_RAW DB '????????????'
       DB 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
MEMTOP	DW	0			;exclusive memory limit, capped at FFFFh
CMD@TBL DB 'O'
	DW Offset HB_TOGGLE
	DB ' '			;if 'space' or..
	DW	(Offset FORWARD)
	DB	CR			;..'cursor return', move to next file.
	DW	(Offset FORWARD)
	DB	'B'			;if reverse, subtract one ring position.
	DW	(Offset REVERSE)
	DB	BS
	DW	(Offset REVERSE)
	DB	'C'			;copy file to another disk?
	DW	(Offset COPY)
	DB	'D'			;delete a file?
	DW	(Offset DELETE)
	DB	'L'			;show file length (size)?
	DW	(Offset FIL@SIZ)
	DB	'G'			;goto filename?
	DW	(Offset GOTO)
	DB	'J'			;jump forward?
	DW	(Offset JUMP22)
	DB	'N'			;new 'dir'ectory, login another disk?
	DW	(Offset LOG)
	DB	'M'			;tagged multiple file copy?
	DW	(Offset MASS)
	DB	'P'			;output file to 'list' device?
	DW	(Offset LSTFILE)
	DB	'R'			;if rename, get to work.
	DW	(Offset RENAME)
	DB	'S'			;free bytes on..
	DW	(Offset R@DR@ST)	;..requested disk?
	DB	'T'			;if tag, put '*' in..
	DW	(Offset TAG@EM)		;..front of cursor.
	DB	'U'			;remove '*' from..
	DW	(Offset UNTAG)		;..in front of cursor?
	DB	'V'			; 'view' file at console?
	DW	(Offset VIEW)
	DB	'X'			;if exit, then to cp/m ccp.
	DW	(Offset CPM@CCP)
	DB	ESCCHR			; 'esc' exits to cp/m ccp also.
	DW	(Offset CPM@CCP)
	DB	0			;table ending
; data and flags
JOKER	DB	'???????????'		; *.* equivalent
J@FLG	DB	TRUE			;default jump 22-files command flag
FIRST@M	DB	FALSE			; 1st time thru in mass copy mode
MFLAG	DB	TRUE			;mass copy flag --> 0 for group copy
TAG@TOT	DW	0			;summation of tagged file sizes
CMDBUF	DB	32,0			;command buffer maximum length, usage, and..
; uninitialized
	RS	100			;..storage for buffer and local stack.
STACK	RS	2			;cp/m's stack pointer stored here
B@MAX	RS	2			;highest block number on disk
B@MASK	RS	1			;sec/blk - 1
BSHIFTF	RS	1			; # of shifts to multiply by sec/blk
BUF@PT	RS	2			;copy buffer current pointer..
BUFSTART RS	2			;..and begin pointer.
CANFLG	RS	1			;no-file-found cancel flag
C@DR	RS	1			; 'current drive'
CHARCNT	RS	1			;character count for tab expansion
CON@LST	RS	1			;bdos function storage
CRCTBL	RS	512			;tables for 'crc' calculations
CRCVAL	RS	2			; 2-byte 'crc' value of working file and..
CRCVAL2	RS	2			;..of finished source read-file.
C@U@A	RS	1			; 'current user area'
D@FCB	RS	33			;fcb for destination file (new name if rename)
EOFLAG	RS	1			;file copy loop 'eof' flag
FS@FLG	RS	1			;tag total versus file size flag
J@CNT	RS	1			;jump forward file counter
LPSCNT	RS	1			;lines-per-screen/page counter for 'v' & 'p'
O@USR	RS	1			;store initial user area for exit
R@DR	RS	1			; 'requested drive'
RCNT	RS	2			; # of records in file and..
REC@CNT	RS	2			;..currently in ram buffer.
REC@MAX	RS	2			; 128-byte record capacity of copy buffer
RINGI	RS	2			;ring alpha sort..
RINGJ	RS	2			;..pointers.
RINGEND	RS	2			;ring end pointer and..
RINGPOS	RS	2			;current ring position in scan.
R@U@A	RS	1			; 'requested user area'
S@FCB	RS	36			;fcb for source (random record) file
TEST@RT	RS	1			;intermediate right-justify data
T@UN@FG	RS	1			;tag/untag file summation switch
VIEWFLG	RS	1			; 00h --> to 'print' else to crt 'view'
; cp/m system functions
RDCON	EQU	1			;console input function
WRCON	EQU	2			;write character to console..
PUNCH	EQU	4			;..punch and..
LISTDEV	EQU	5			;..to list logical devices.
DIRCON	EQU	6			;direct console i/o
RDBUF	EQU	10			;read input string
CONST	EQU	11			;get console status
GETVERS	EQU	12			;get cp/m version
RESETDK	EQU	13			;reset disk system
LOGIN	EQU	14			;login new disk
OPEN	EQU	15			;open file
CLOSE	EQU	16			;close file
SRCHF	EQU	17			;search directory for first..
SRCHN	EQU	18			;..and next occurrance.
ERASE	EQU	19			;erase file
READ	EQU	20			;read and..
WRITE	EQU	21			;..write 128-record.
MAKE	EQU	22			;make file
REN	EQU	23			;rename file
INQDISK	EQU	25			;get current (default) drive
SETDMA	EQU	26			;set dma address
INQALC	EQU	27			;allocation vector
GETPARM	EQU	31			;current drive parameters address
SGUSER	EQU	32			;set or get user area
COMPSZ	EQU	35			; # of records in file
FRESPC	EQU	46			;get disk free space
; system addresses
BDOS	EQU	CPM@BASE+05H		;bdos function entry address
FCB	EQU	CPM@BASE+5CH		;default file control block
FCBEXT	EQU	FCB+12			;extent byte in 'fcb'
FCBRNO	EQU	FCB+32			;record number in 'fcb'
TBUF	EQU	CPM@BASE+80H		;default cp/m buffer
; assembled 'com' and 'ram-loaded' file size (0c00h = 3k)
COMFILE	EQU	((Offset CMDBUF)+2)-256	; 'prn' listing shows 'com'..
LASTADR	EQU	(Offset $)
RING	EQU	(Offset LASTADR)+100H AND 0FF00H
