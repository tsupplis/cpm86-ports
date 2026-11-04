; determine if specific file(s) requested -- show remaining storage
EMBARK:	CALL	FRESTOR			;get bytes remaining on drive (decode default)
	MOV	AL,Byte Ptr .FCB+1	;check if a filename was entered
	CMP	AL,' '			;filename a space?
	JNZ	PLUNGE			;no, name was entered.
	MOV	AL,Byte Ptr .FCB+9	;filetype also space?
	CMP	AL,' '			;if so, then..
	JNZ	PLUNGE
	MOV	BX,(Offset JOKER)	;..treat as '*.*' with 'joker'..
	MOV	DX,FCB+1		;..loaded here.
	MOV	CH,11			; # of characters to move
	CALL	MOVE			;set field to *.*
; build 'ring' with filename positioned in default 'fcb' area
PLUNGE:
	CALL HB_PROBE
	MOV	CL,SETDMA		;initialize dma address..
	MOV	DX,TBUF			;..to default buffer.
	INT	224
	XOR	AL,AL			;clear search 'fcb'..
	MOV	Byte Ptr .FCBEXT,AL	;..extent byte..
	MOV	Byte Ptr .FCBRNO,AL	;..and record number.
	NOT	AL
	MOV	Byte Ptr CANFLG,AL	;make cancel flag true
	MOV	DX,FCB			;default 'fcb' for search..
	MOV	CL,SRCHF		;..of first occurrence.
	INT	224
	INC	AL			; 0ffh --> 00h if no file found
	JZ	L_3	
	JMP	SETRING			;if found, branch and build ring.
L_3:
	MOV	Byte Ptr CANFLG,AL	;make log-cancel toggle false
	CALL	ILPRT			;else say none found, fall thru to log.
	DB	CR,LF,'++ NO FILE FOUND ++',BELL,0
TO@MCL:	CALL	ILPRT			;ask to log or exit
	DB	CR,LF,LF,' ---> Command (n/x): ',0
	CALL	KEYIN			;if not exit, assume..
	CMP	AL,'X'			;..login to new drive/user area.
	JNZ	L_4	
	JMP	CPM@CCP
L_4:
; l o g
; select drive and user area (system reset for disk change on-the-fly)
LOG:	CALL	ILPRT			;prompt to get drive/user selection
	DB	'ew DIRectory: ',0
	CALL	DEF@D@U
	MOV	AL,Byte Ptr R@U@A	;establish requested area..
	MOV	Byte Ptr C@U@A,AL	;..as current area.
	CALL	SET@USR
	CALL	RESET			;reset disk system, make requested current.
	MOV	AL,' '			;set default 'fcb' to look like *.*
	MOV	Byte Ptr .FCB+1,AL
	MOV	Byte Ptr .FCB+9,AL
	MOV	BX,0			;initialize tagged..
	MOV	Word Ptr TAG@TOT,BX	;..file size accumulator.
	CALL	ILPRT
	DB	CR,LF,LF,0		;fresh line and..
	JMP	EMBARK			;..restart.
; routine to define current drive and user area with full error trapping.
; (check validity of user area entry first, then drive validity, then proceed
; with implementation.)
DEF@D@U:MOV	BX,(Offset CMDBUF)+2
	MOV	CH,7			; # of blanks to..
	CALL	FILL			;..clear 'cmdbuf'.
	MOV	DX,(Offset CMDBUF)	;get drive/user selection from..
	MOV	CL,RDBUF		;..console buffer read.
	INT	224
	CALL	CONVERT			;make sure alpha is upper case
	XOR	AL,AL			;initialize..
	MOV	Byte Ptr R@U@A,AL	;..user area to zero.
	MOV	AL,Byte Ptr CMDBUF+3	; 1st digit of user area?
	CMP	AL,':'			;allow ':' after drive declaration
	JZ	SETEXIT
	CMP	AL,'0'			;if no valid user area request..
	JB	SETEXIT			;..then to new drive and ring list.
	CMP	AL,'9'+1
	JNB	ERRET			;error, not a user area.
	SUB	AL,30H			;convert to binary and..
	CMP	AL,1			;..test if 10's digit.
	JNZ	SETUSER			;if none, then set user area now.
	MOV	AL,Byte Ptr CMDBUF+4	;a second user area digit?
	CMP	AL,':'			;allow ':' here
	JZ	SETUONE
	CMP	AL,'0'			;test for 1's digit
	JB	SETUONE
	CMP	AL,'5'+1		;if user area >15, go..
	JNB	ERRET			;..error msg, show file line.
	SUB	AL,30H-10		;make 1 --> 11, 2 --> 12, etc.
	MOV	Byte Ptr R@U@A,AL	;save as 'requested user area' here..
	JMPS	SETEXIT
SETUONE:MOV	AL,1			;set to user area 'one'
SETUSER:MOV	CH,AL
	MOV	AL,Byte Ptr CMDBUF+4
	CMP	AL,':'			;double dot (colon)?
	JZ	DDPASS
	CMP	AL,'0'			;if >19 user area, go error msg.
	JNB	ERRET
DDPASS:	MOV	AL,CH
	MOV	Byte Ptr R@U@A,AL	;..and here.
SETEXIT:MOV	AL,Byte Ptr MAXDR	;check if system maximum and..
	INC	AL
	MOV	CH,AL
	MOV	AL,Byte Ptr CMDBUF+2	;..requested drive are compatible.
	CMP	AL,CH			;if input too big..
	JNB	ERRET			;..or..
	MOV	CH,'A'-1		;..too..
	CMP	AL,CH			;..small, show..
	JB	ERRET			;..error msg.
	SUB	AL,'A'-1		;ready for fcb use
	MOV	Byte Ptr .FCB,AL	;store 1 --> a:, 2 --> b:, etc.
	DEC	AL
	MOV	Byte Ptr R@DR,AL	;ready for 'login' request
	RET
; error return and recovery from command cancellation
ERRET:	CALL	ILPRT
	DB	CR,LF,'++ Drive/User Entry Error ++',BELL,0
COMCAN:	MOV	SP,(Offset STACK)	;reset stack..
	MOV	AL,Byte Ptr CANFLG
	OR	AL,AL			;..from..
	JNZ	L_5	
	CALL	CRLF
L_5:
	JNZ	L_6	
	JMP	PLUNGE
L_6:
	JMP	NEUTRAL			;..error/command abort.
; e x i t
; return to cp/m ccp
CPM@CCP:MOV	AL,Byte Ptr O@USR	;get and set original..
	CALL	SET@USR			;..user area and..
	MOV	DX,TBUF			;..tidy up..
	MOV	CL,SETDMA		;..before going home.
	INT	224
	CALL	CRLF
; CP/M-86: terminate through BDOS 0 instead of returning to the CCP
	MOV	CL,0
	INT	224
;not warmboot
; h e l p  (menu)
HELP:	CALL	CLS			;show menu but 'clear-screen' first
	CALL	ILPRT
	DB	CR,'                 DISK '
	DB	VERS/10+'0','.',VERS MOD 10+'0'
	DB	' -- File Manipulation Program --'
	DB	' '
	DB	MONTH/10+'0',MONTH MOD 10+'0','/'
	DB	DAY/10+'0',DAY MOD 10+'0','/'
	DB	YEAR/10+'0',YEAR MOD 10+'0'
	DB	CR,LF
	DB	'   C - Copy file     | D - Delet'
	DB	'e file   | L - Length of Fil'
	DB	'e | G - Goto fn.ft',CR,LF
	DB	'   J - Jump 22 files | M - Mass '
	DB	'transfer | N - New DIRectory'
	DB	'  | P - Print text',CR,LF
	DB	'   R - Rename file   | S - Stat '
	DB	'of disk  | T - Tag file     '
	DB	'  | U - Untag file',CR,LF
	DB	'   V - View text     | X - Exit '
	DB	'to CP/M  | <SP> advances cur'
	DB	'sor -- B backs up'
	DB CR,LF,'   O - Toggle normal/all; S/H after names identify attributes'
	DB	CR,LF,LF,0
	RET
; establish ring (circular list) of filenames -- set-up sort pointers
SETRING:MOV	BX,(Offset RING)	;initialize ring pointer
	MOV	Word Ptr RINGPOS,BX	;start --> current position of ring
	MOV	Word Ptr RINGI,BX	;initialize sort 'i' and..
	MOV	DX,HB_STRIDE			;..'j' variables.
	ADD	BX,DX
	MOV	Word Ptr RINGJ,BX
; put each found name in ring.	(a-reg --> offset into 'tbuf' name storage)
TO@RING:DEC	AL			;un-do 'inr' from above and below
	ADD	AL,AL			;times 32 --> position index
	ADD	AL,AL
	ADD	AL,AL
	ADD	AL,AL
	ADD	AL,AL
	ADD	AL,TBUF			;add page offset and..
	MOV	BL,AL			;..put address into..
	MOV	BH,0			;..hl-pair.
	CALL HB_FLAGS
	JNC HB_KEEP
	JMP HB_SEARCH_NEXT
HB_KEEP:
	MOV	DX,Word Ptr RINGPOS
	ADD	DX,HB_STRIDE+128		;leave one record for copying
	JC	HB_LIST_FULL
	CMP	DX,Word Ptr MEMTOP
	JBE	HB_LIST_FITS
HB_LIST_FULL:
	CALL	ILPRT
	DB	CR,LF,' ++ Directory too large for memory ++',CR,LF,0
	JMP	CPM@CCP
HB_LIST_FITS:
	MOV	AL,Byte Ptr .FCB	;get drive/user designator and..
	MOV	M,AL			;..put into 'fcb' buffer.
	XCHG	BX,DX
	MOV	BX,Word Ptr RINGPOS	;pointer to current load point in ring
	XCHG	BX,DX
	MOV	CH,12			;move drive designator and name to ring
	CALL	MOVE
	XCHG	BX,DX			;de-pair contains next load point address
	MOV	M,' '			;space for potential..
	INC	BX			;..tagging of files for mass copy.
	MOV AL,Byte Ptr HB_LAST
	MOV M,AL
	INC BX
	MOV	Word Ptr RINGPOS,BX	;store and search..
HB_SEARCH_NEXT:
	MOV	CL,SRCHN		;..for next occurrence.
	MOV	DX,FCB			;filename address field
	INT	224
	INC	AL			;if all done, 0ffh --> 00h.
	JZ	HB_LIST_DONE
	JMP	TO@RING			;if not, put next name into ring.
HB_LIST_DONE:
; all filenames in ring -- setup ring size and copy-buffer start point
	MOV	BX,Word Ptr RINGPOS	;next load point of ring is start of buffer
	MOV	Word Ptr RINGEND,BX	;set ring end..
	MOV	Word Ptr BUFSTART,BX	;..and copy-buffer start.
	CMP BX,Offset RING
	JNE HB_HAVE_FILES
	JMP HB_EMPTY
HB_HAVE_FILES:
	MOV	DX,(Offset RING)+HB_STRIDE	;compare 'ringend' (tab base+13)
	CALL	CMPDEHL
	JNZ	L_7	
	JMP	L_8			;one file still needs CRC and buffer setup
L_7:
; sort ring of filenames
SORT@LP:MOV	BX,Word Ptr RINGJ	;compare names 'i & j'
	XCHG	BX,DX
	MOV	BX,Word Ptr RINGI
	PUSH	BX			;save position pointers..
	PUSH	DX			;..for potential swap.
	MOV	CH,HB_STRIDE			; # of characters to compare
; left to right compare of two strings (de-pair points to 'a' string;
; hl-pair, to 'b'; b-reg contains string length.)
CMPSTR:	MOV	SI,DX			;get an 'a' string character and..
	MOV	AL,[SI]
	CMP	AL,M			;..check against 'b' string character.
	JNZ	NOCMP			;if not equal, set flag.
	INC	BX			;bump compare..
	INC	DX			;..pointers and.. (if compare, set as equal.)
	DEC	CH
	JNZ	CMPSTR			;..do next character.
NOCMP:	POP	DX
	POP	BX
	MOV	CH,HB_STRIDE
	JNB	NOSWAP
; swap if 'j' string larger than 'i'
SWAP:	MOV	CL,M			;get character from one string..
	MOV	SI,DX			;..and one from other string.
	MOV	AL,[SI]
	MOV	M,AL			;second into first
	MOV	AL,CL			;first into second
	MOV	SI,DX
	MOV	[SI],AL
	INC	BX			;bump swap pointers
	INC	DX
	DEC	CH
	JNZ	SWAP			;all bytes swapped yet?
NOSWAP:	MOV	BX,Word Ptr RINGJ	;increment 'j' pointer
	MOV	DX,HB_STRIDE
	LAHF
	ADD	BX,DX
	SAHF
	MOV	Word Ptr RINGJ,BX
	XCHG	BX,DX			;see if end of 'j' loop
	MOV	BX,Word Ptr RINGEND
	CALL	CMPDEHL
	JNZ	SORT@LP			;no, so more 'j' looping.
	MOV	BX,Word Ptr RINGI	;bump 'i' pointer
	MOV	DX,HB_STRIDE
	LAHF
	ADD	BX,DX
	SAHF
	MOV	Word Ptr RINGI,BX
	LAHF				;set start over 'j' pointer
	ADD	BX,DX
	SAHF
	MOV	Word Ptr RINGJ,BX
	XCHG	BX,DX			;see if end of 'i' loop
	MOV	BX,Word Ptr RINGEND
	CALL	CMPDEHL
	JZ	L_8	
	JMP	SORT@LP			;must be more 'i' loop to do
L_8:
; sort done -- initialize tables for fast crc calculations
	CALL	INITCRC
; calculate copy-buffer maximum available record capacity
B@SIZE:	MOV	AX,Word Ptr MEMTOP
	SUB	AX,Word Ptr BUFSTART
	JNC	B@SIZE2
	XOR	AX,AX
B@SIZE2:
	MOV	CL,7
	SHR	AX,CL			;whole 128-byte records within allocation
	MOV	CX,AX
	MOV	AL,CH			;memory available for copy?
	OR	AL,CL
	JNZ	B@SIZE3			;yes, buffer memory space available.
	CALL	ILPRT
	DB	CR,LF,'No Buffer Space',0
	JMP	NEUTRAL
B@SIZE3:MOV	BL,CL			;store..
	MOV	BH,CH			;..maximum..
	MOV	Word Ptr REC@MAX,BX	;..record count of copy buffer.
; buffer size suitable -- process file/display loop
