;*******************************************************
	page 60,132
;
;			XLT86
;
; Translates Intel 8080 assembly language source code
;  to Intel 8086 assembly language source code.
;
; 11/11/84 Frank J. Zerilli
;
; 8086 version, 11/20/84
;
;
; MS-DOS version 12/20/84
;  by Craig Derouen

VERS	EQU	110
;
;*******************************************************
;
;      XLT86 processes lines with the exclamation point
; statement separator correctly.  It strips trailing
; blanks or tabs from lines.  It replaces initial
; asterisks in lines with semicolons.  It provides
; several options to format the output file for best
; appearance.
; 
;      This program gives the choice of converting the
; case of instructions to upper or lower case or of
; trying to preserve the case of instructions.
; 
;      An activity dot is printed on the console for
; every 100 lines of input processed.
; 
;
; 
; Command line:
;
;     XLT86 [d:]srcfile[.typ] [d:destfile.typ]
;
;      All parameters in brackets are optional, and, if
; omitted, the default values are:
;
;  Source file-type      -- ASM
;  Destination file-type -- A86
;  Destination file-name -- same as the source file-name
;  Drive                 -- current drive
;
; FLAG LOCATIONS:
;
; 103H -- Change to non-zero value to suppress
;	  translation of several non-standard opcodes:
;	  REQ, RNE, RLT, RGE, CEQ, CNE, CLT, CGE
;	  JEQ, JNE, JLT, JGE,
;	  ENT, NAM, RAM, ROG, IFC, ICL, LST, MAC
;
; 104H -- If non-zero (default) XLT86 converts lines
;	  with multiple statements separated by DR's EP
;	  separator into separate lines.
;	  Change to zero for output on a single line
;	  with the translated statements separated by
;	  EP.
;
; 107H--  If zero (default) XLT86 translates
;
;		PUSH PSW	POP PSW
;
;	  to
;
;		LAHF		POP AX
;		PUSH AX		SAHF
;
;	  Otherwise, the translation is
;
;		LAHF		POP AX
;		XCHG AH,AL	XCHG AH,AL
;		PUSH AX		SAHF
;		XCHG AH,AL
;
; 108H-- If zero (default) XLT86 translates
;
;		INX rp		DCX rp
;
;	 to
;
;		INC rp'		DEC rp'
;
;	 Otherwise, the translation is
;
;		PUSHF		PUSHF
;		INC rp'		DEC rp'
;		POPF		POPF
;
; 109H-- If zero (default) XLT86 translates
;
;		DAD rp
;
;	 to
;
;		ADD BX,rp'
;
;	 Otherwise, the translation is
;
;		PUSHF
;		ADD BX,rp'
;		POPF
;

N00	EQU	0
N01	EQU	1
;
N07	EQU	7
N09	EQU	9		;tab every 8th col.
NF8	EQU	0F8H		;mod 8
;
LBUFLN	EQU	80		;line buffer length
OPBFLN	EQU	5		;opcode buffer length
MEMSIZ	EQU	4		;memory available in Kb
IBNUM	EQU	4*MEMSIZ
OBNUM	EQU	4*MEMSIZ
RECLEN	EQU	128		;
STCKLN	EQU	128		;stack size
IBFLEN	EQU	IBNUM*RECLEN
OBFLEN	EQU	OBNUM*RECLEN
;
CTRLC	EQU	3
EOT	EQU	4
BEL	EQU	7
HT	EQU	9
LF	EQU	0AH
CR	EQU	0DH
ESC	EQU	1BH
QUOTE	EQU	27H
;

code	segment byte

	assume cs:code,ds:code

	ORG	005CH
DFCB1	DB	16 DUP(?)
DFCB2	DB	16 DUP(?)
;
FNLEN	EQU	8
EOS	EQU	EOT		; replacement for exclamation pt
EOF	EQU	1AH
NFF	EQU	0FFH		;disk error return
;
;	BDOS FUNCTIONS
;
nABT	EQU	0
nCIN	EQU	1
nCOUT	EQU	2
nCDAV	EQU	0BH
nOPEN	EQU	0FH
nCLOSE	EQU	10H
nDEL	EQU	13H
nRDNR	EQU	14H
nWRNR	EQU	15H
nCREAT	EQU	16H
nCDISK	EQU	19H
nDMA	EQU	1AH
;
;
;
	ORG	100H
;
HERE_FIRST:

	JMP	START
;
; OPTION FLAGS
;
PSEFLG	DB	0		;(103H) 0 to translate non-
				;  standard opcodes
MLTLFL	DB	0FFH		;(104H) 0 to put input line with
				;  exc. pt. to output on one line
TCASFL	DB	0		;(105H) 0 to preserve case
				;
LCASFL	DB	0		;(106H) 0 for upper case if
				;  TCASFL not zero
PSWFL	DB	0		;(107H) non-zero to preserve 8080
				;  order of PSW registers on stack
INXFL	DB	0		;(108H) non-zero to preserve flags
				;  with INX and DCX translations
DADFL	DB	0		;(109H) non-zero to preserve flags
				;  with DAD translation
				
;
; BDOS Functions
;
BDOS:
	INT	21H
	RET

;
; RETURN TO MSDOS
;
ABORT:	MOV	AH,4ch
	INT	21h

;
;  HELP MESSAGE
;
HMSG1:

 DB CR,LF
 DB LF
 DB 'XLT86 translates Intel 8080 assembly language source',CR,LF
 DB 'code into Intel 8086 assembly language source code.',CR,LF
 DB LF
 DB 'It is invoked by a command of the form:',CR,LF
 DB LF
 DB '    XLT86 [d:]srcfile[.typ] [d:destfile.typ]',CR,LF
 DB LF
 DB 'The brackets denote optional parameters and the ',CR,LF
 DB 'default values are:',CR,LF
 DB LF
 DB '    Source file-type      --  ASM',CR,LF
 DB '    Destination file-type --  A86',CR,LF
 DB '    Destination file-name --  same as source file-name',CR,LF
 DB '    Drive                 --  current drive',CR,LF
 DB CR,LF
 DB 'Press any key to continue - ',0

HMSG2:

 DB CR,LF
 DB LF
 DB 'Examples:',CR,LF
 DB LF
 DB 'XLT86 PRGM1               --translates PRGM1.ASM to PRGM1.A86',CR,LF
 DB 'XLT86 PRGM1 PRGM2         --translates PRGM1.ASM to PRGM2.A86',CR,LF
 DB 'XLT86 PRGM1.TXT PRGM2.MAC --translates PRGM1.TXT to PRGM2.MAC',CR,LF
 DB LF
 DB 'XLT86 also has the following features:',CR,LF
 DB LF
 DB 'Case will be preserved as well as possible -- if an opcode has',CR,LF
 DB 'a lower case character, the translated opcode will be in lower',CR,LF
 DB 'case.',CR,LF
 DB LF
 DB 'All asterisks at the beginning of lines will be replaced with',CR,LF
 DB 'semicolons.',CR,LF
 DB LF
 DB 'A dot is printed on the console for every 100 lines of input ',CR,LF
 DB 'processed.',CR,LF
 DB LF
 DB 0
;
;=============================================================================
;
; Program begins here
;
START:
	MOV	AX,SS
	MOV	SSBDOS,AX
	MOV	SPBDOS,SP

	MOV	AX,CS
	MOV	SS,AX
	MOV	SP,OFFSET STACK

	MOV	AL,DFCB1+1	; check for a file name
	CMP	AL,' '		; print help if no name
	JNZ	BEGIN		; no help requested
	MOV	DX,OFFSET SIGNON
	CALL	PRTLIN
	MOV	DX,OFFSET HMSG1	; print help message
	CALL	PRTLIN
	MOV	AH,nCIN		; wait for any character
	CALL	BDOS
	MOV	DX,OFFSET HMSG2	; print rest of help
	CALL	PRTLIN

	MOV	AX,SSBDOS
	MOV	SS,AX		; retrieve system stack
	MOV	SP,SPBDOS	;  pointer and pop
	MOV	al,2		; put in an error code
	MOV	AH,4CH
	INT	21H		; Return to DOS

BEGIN:	CALL	HELLO		;signon, open in & out files
NXTLIN:	CALL	GETLIN		;get line from input file to buf
	CALL	PROCLIN		; process line
	JMP	NXTLIN
;
;
;

;*******************************************************
;
;	Print signon, open input
;	and output files.
;
HELLO:	MOV	DX,OFFSET SIGNON
	CALL	PRTLIN
HELLO0:	MOV	AL,'D'		; translate DB & EQU (for
	MOV	OPTBDB,AL	;  uniform formatting)
	MOV	MLTSPC,HT	; for opcodes xltd to mlt stmts
	MOV	AL,HT		; HT after opcode
	MOV	BX,OFFSET PUTHT+1
	MOV	[BX],AL
	MOV	AL,41		; tab comments to col 33
	MOV	BX,OFFSET PUTND5+1
	MOV	[BX],AL
	MOV	AL,3CH		; CMP instruction
	MOV	BX,OFFSET EXCLAM
	MOV	[BX],AL
	XOR	AL,AL
	MOV	TCASFL,AL	; don't convert case
	MOV	LCASFL,AL	; lower case flag
	MOV	SEPMSG,OFFSET NEWLSP

	MOV	AL,PSEFLG	; translate non-standard
	OR	AL,AL		;  opcodes ?
	JZ	LBL1
	 CALL NXPSD
LBL1:

	MOV	DX,OFFSET DBMSG
	CALL	PRTLIN		;
	CALL	CHKYES		; xlat DB & EQU ?
	CMP	AL,ESC
	JNZ	LBL2
	 JMP HELLO0
LBL2:
	CMP	AL,CTRLC
	JNZ	LBL3
	 JMP ABORT
LBL3:
	CMP	AL,'Y'
	JZ	LBL4
	 CALL NXDBEQ
LBL4:

	MOV	AL,MLTLFL	; force space after opcode
	OR	AL,AL		;  if MLTLFL not set
	JNZ	LBL5
	 JMP HELLO2
LBL5:
	MOV	DX,OFFSET SPCMSG
	CALL	PRTLIN		; use space after
	CALL	CHKYES		; opcode ?
	CMP	AL,ESC
	JNZ	LBL6
	 JMP HELLO0
LBL6:
	CMP	AL,CTRLC
	JNZ	LBL7
	 JMP ABORT
LBL7:
	CMP	AL,'Y'
HELLO2:	JNZ	LBL8
	 CALL SETSPC
LBL8:

	MOV	DX,OFFSET COLMSG
	CALL	PRTLIN		;
	CALL	CHKYES		; start comment
	CMP	AL,ESC
	JNZ	LBL9
	 JMP HELLO0
LBL9:
	CMP	AL,CTRLC
	JNZ	LBL10
	 JMP ABORT
LBL10:
	CMP	AL,'Y'
	JNZ	LBL11
	 CALL SETCOL ; in column 25 ?
LBL11:

	MOV	DX,OFFSET EXCMSG
	CALL	PRTLIN		; Ignore exclamation point
	CALL	CHKYES		;  separator ?
	CMP	AL,ESC
	JNZ	LBL12
	 JMP HELLO0
LBL12:
	CMP	AL,CTRLC
	JNZ	LBL13
	 JMP ABORT
LBL13:
	CMP	AL,'Y'
	JNZ	LBL14
	 CALL SETNEX
LBL14:

	MOV	DX,OFFSET MLTMSG
	CALL	PRTLIN		; multiple statements
	CALL	CHKYES		; on one line ?
	CMP	AL,ESC
	JNZ	LBL15
	 JMP HELLO0
LBL15:
	CMP	AL,CTRLC
	JNZ	LBL16
	 JMP ABORT
LBL16:
	CMP	AL,'Y'
	JNZ	LBL17
	 CALL SETMLT
LBL17:

	MOV	DX,OFFSET TRNMSG ; Convert case ?
	CALL	PRTLIN
	CALL	CHKYES
	CMP	AL,ESC
	JNZ	LBL18
	 JMP HELLO0
LBL18:
	CMP	AL,CTRLC
	JNZ	LBL19
	 JMP ABORT
LBL19:
	CMP	AL,'L'
	JNZ	LBL20
	 CALL SETLC
LBL20:
	CMP	AL,'U'
	JNZ	LBL21
	 CALL SETUC
LBL21:

	MOV	AL,N01
	MOV	COLNUM,AL

	MOV	AH,nCDISK
	CALL	BDOS
	INC	AL
	MOV	xCDISK,AL

	CALL	MAKFNS
	CALL	OPENIN
	CALL	CREATO
	RET
;
SIGNON	DB	'8080-to-8086 Translator version '
	DB	VERS/100+'0','.',(VERS MOD 100)/10+'0'
	DB	(VERS MOD 10)+'0'
	DB	CR,LF,0
;
; Don't translate non-standard opcodes
;
NXPSD:	XOR	AL,AL
	MOV	OPTTDL,AL
	MOV	OPTENT,AL
	MOV	OPTIFC,AL
	RET

DBMSG	DB	'Translate DB & EQU ? '
	DB	'[Y/ret=N/esc/^C] ',0
;
NXDBEQ:	XOR	AL,AL
	MOV	OPTBDB,AL
	RET
;
SPCMSG	DB	'Use space (default TAB) after opcode ? '
	DB	'[Y/ret=N/esc/^C] ',0
;
SETSPC:	MOV	BX,OFFSET PUTHT+1
	MOV	AL,' '
	MOV	[BX],AL
	MOV	MLTSPC,AL
	RET
;
COLMSG	DB	'Start comment in column 25 (default 33) ? '
	DB	'[Y/ret=N/esc/^C] ',0
;
SETCOL:	MOV	BX,OFFSET PUTND5+1
	MOV	AL,33
	MOV	[BX],AL
	RET
;
EXCMSG	DB	'Ignore statement separator ? '
	DB	'[Y/ret=N/esc/^C] ',0
;
SETNEX:	MOV	AL,0C3H		; RET instruction
	MOV	BX,OFFSET EXCLAM
	MOV	[BX],AL
	RET
;
MLTMSG	DB	'Put opcodes converted to multiple'
	DB	' statements on one line ? '
	DB	'[Y/ret=N/esc/^C] ',0
;
SETMLT:	MOV	SEPMSG,OFFSET EXCLSP
	MOV	MLTSPC,' '
	RET

;
TRNMSG	DB	'Translate instructions to Upper/Lower'
	DB	' or preserve case ? [U/L/ret/esc/^C] ',0
;
SETLC:	MOV	TCASFL,AL
	MOV	LCASFL,AL
	RET
;
SETUC:	MOV	TCASFL,AL
	RET

;*******************************************************
;
;	Gets line from input file to line
;	buffer until CR.  Filters out ctrl
;	chars except for HT.  Truncates
;	lines after LBUFLN chars.
;	Terminates line with CR, LF, 0.
;
GETLIN:	CALL	PDOT		; print activity dot
	CALL	CHKIN
	CMP	AL,CTRLC
	JNZ	LBL22
	 JMP JABORT
LBL22:
	XOR	AL,AL
	MOV	QUOTFL,AL	; not in quote
	MOV	CMNTFL,AL	; not in comment
	MOV	BX,OFFSET LBUFF	;line buffer
	MOV	CH,LBUFLN	;max # of char
GETLN1:	XCHG	BX,DX
	MOV	BX,XIBUFF
	XCHG	BX,DX

	MOV	AL,DH
	CMP	AL,(SIZE IBUFF)/256
	JZ	LBL23
	 JMP GETLN4
LBL23:
	MOV	AL,DL
	CMP	AL,(SIZE IBUFF) MOD 256

	JZ	LBL24
	 JMP GETLN4
LBL24:
	PUSH	CX
	PUSH	BX
	MOV	DX,OFFSET IBUFF
GETLN2:	MOV	AH,nDMA
	PUSH	DX
	CALL	BDOS
	POP	DX

	XCHG	BX,DX

	MOV	DX,OFFSET INFCB
	MOV	AH,nRDNR
	PUSH	BX
	CALL	BDOS
	POP	BX
	DEC	AL
	JZ	LBL25
	 JMP GETLN3
LBL25:
	MOV	AL,EOF
	MOV	[BX],AL
GETLN3:	MOV	DX,OFFSET RECLEN
	ADD	BX,DX
	XCHG	BX,DX
	MOV	AL,DH
	CMP	AL,(SIZE IBUFF)/256
	JZ	LBL26
	 JMP GETLN2
LBL26:
	MOV	AL,DL
	CMP	AL,(SIZE IBUFF) MOD 256
	JZ	LBL27
	 JMP GETLN2
LBL27:

	POP	BX
	POP	CX
	MOV	DX,OFFSET IBUFF
GETLN4:	XCHG BX,DX 
	 MOV AL,[BX] 
	XCHG BX,DX
	INC	DX
	XCHG	BX,DX
	MOV	XIBUFF,BX
	XCHG	BX,DX
	MOV	[BX],AL

	CMP	AL,QUOTE	; set or reset
	JZ	LBL28
	 JMP GTLN41 ;  QUOTFL
LBL28:
	MOV	AL,QUOTFL
	NOT	AL
	MOV	QUOTFL,AL

GTLN41:	MOV	AL,[BX] ; Translate exclam.  pt.
	CALL	EXCLAM		;  which is not in quote
	MOV	[BX],AL ;  to EOS

	MOV	AL,TCASFL	; translate to upper
	OR	AL,AL		;  or lower case ?
	JNZ	LBL29
	 JMP GTLN46 ; NO
LBL29:

	MOV	AL,QUOTFL	; if in quote, do
	OR	AL,AL		;  nothing
	JZ	LBL30
	 JMP GTLN43
LBL30:
	MOV	AL,[BX] ; otherwise, ';' sets
	CMP	AL,';'		;  CMNTFL and EOS resets
	JNZ	LBL31
	 JMP GTLN42 ;  it
LBL31:
	CMP	AL,EOS
	JZ	LBL32
	 JMP GTLN43
LBL32:
	XOR	AL,AL
GTLN42:	MOV	CMNTFL,AL

GTLN43:	MOV	AL,QUOTFL	; If in quote,
	OR	AL,AL		;  do nothing
	JZ	LBL33
	 JMP GTLN46
LBL33:
	MOV	AL,CMNTFL	; If in comment,
	OR	AL,AL		;  do nothing
	JZ	LBL34
	 JMP GTLN46
LBL34:
	MOV	AL,LCASFL	; Otherwise,
	OR	AL,AL		;  if LCASFL set
	MOV	AL,[BX]
	JNZ	LBL35
	 JMP GTLN44
LBL35:
	CALL	LCASE		;  trns to lwr case
	JMP	GTLN45
GTLN44:	CALL	UCASE		;  else trns to upr case
GTLN45:	MOV	[BX],AL

GTLN46:	MOV	AL,[BX]
	CMP	AL,CR
	JNZ	LBL36
	 JMP GETLN6
LBL36:
	CMP	AL,HT		; filters out all ctrl
	JNZ	LBL37
	 JMP GETLN5 ; chars except tab
LBL37:

	CMP	AL,EOF
	JNZ	LBL38
	 JMP GETLN7
LBL38:
	CMP	AL,EOS
	JNZ	LBL39
	 JMP GETLN5
LBL39:

	CMP	AL,' '
	JNC	LBL40
	 JMP GETLN1
LBL40:
GETLN5:	INC	BX
	DEC	CH
	JZ	LBL41
	 JMP GETLN1
LBL41:
	DEC	BX
	INC	CH
	JMP	GETLN1
;
GETLN6:	INC	BX
	MOV	BYTE PTR [BX],LF
	INC	BX
	MOV	BYTE PTR [BX],N00
	XCHG	BX,DX
	MOV	XIBUFF,BX
	XCHG	BX,DX
	RET

;
;	Change exclamation point to EOS in A
;
EXCLAM:	CMP	AL,'!'
	JZ	LBL42
	RET
LBL42:
	MOV	AL,QUOTFL
	OR	AL,AL
	MOV	AL,'!'
	JZ	LBL43
	RET
LBL43:
	MOV	AL,EOS
	RET
;
QUOTFL	DB	0
CMNTFL	DB	0

;
;	Exit
;
GETLN7:	CALL	CLOSEO
	MOV	DX,OFFSET UPSMSG
	CALL	PRTLIN
	MOV	DX,OFFSET ENDIFL
	CALL	PRTLIN
	MOV	DX,OFFSET ICLFLG
	CALL	PRTLIN
	MOV	DX,OFFSET LSTFLG
	CALL	PRTLIN
	MOV	DX,OFFSET MACFLG
	CALL	PRTLIN
	MOV	DX,OFFSET EOJMSG
;
;	Print message at DE and abort
;
EREXIT:	PUSH	DX
	MOV	DX,OFFSET CRLFMG
	CALL	PRTLIN
	POP	DX
	CALL	PRTLIN
	JMP	ABORT
;
JABORT:	MOV	DX,OFFSET JABTMG
	CALL	PRTLIN
	JMP	GETLN7
;
JABTMG	DB	CR,LF,'*** Job Cancelled ***',CR,LF,0
;
UPSMSG	DB	0,LF,'The following operands'
	DB	' have been used in your '
	DB	'source and have not'
	DB	CR,LF
	DB	'been fully translated.  You must '
	DB	'complete the translation using an editor.'
	DB	CR,LF,HT
	DB	'original:',HT,HT
	DB	'must be translated to:'
	DB	CR,LF,0
ENDIFL	DB	0,'IF or IFC',HT,HT,'%IF(exp)THEN(txt1)',CR,LF
	DB	HT,'ELSE',HT,HT,HT,'ELSE(txt2)',CR,LF
	DB	HT,'ENDIF or #ENDIF',HT,HT,'FI'
	DB	CR,LF,0
ICLFLG	DB	0,'ICL'
	DB	CR,LF,0
LSTFLG	DB	0,'LST or LIST'
	DB	CR,LF,0
MACFLG	DB	0,'MACRO or MAC',HT,HT,'%DEFINE(mname[(plist)])'
	DB	CR,LF
	DB	HT,'#macro-call',HT,HT,'%macro-call'
	DB	CR,LF,0
;
EOJMSG	DB	'*** End of Job ***',CR,LF,0

;*******************************************************
;
;	Process line
;
PROCLIN:
	MOV	BX,OFFSET LBUFF
PROCLN0:
	CALL	FNDOPC
	JNZ	PRCLN00
	CALL	PTCOLN		; Put out colon if
	JMP	PUTND6		;  colon flag set

PRCLN00:
	MOV	AL,COLNFL	; Is there a colon
	OR	AL,AL
	JZ	PRCLN02

	MOV	BX,OFFSET OPTPSD ; Don't put colon
	MOV	CX,OPBFLN	 ;  if opcode is
	CALL	SCANOP		 ;  DB, DW, DS, or EQU
	MOV	AL,' '
	JZ	PRCLN01		; Put space
	MOV	AL,':'		;  otherwise, put colon

PRCLN01:
	CALL	PUTCHR

PRCLN02:
	MOV	BX,xWHITE
	CALL	PUTSPT

PROCLN1:
	MOV	BX,OFFSET OPTIMM ;imm or one byte
	MOV	CX,2*OPBFLN
	CALL	SCANOP
	JNZ	LBL44
	 JMP DOIMM
LBL44:

	MOV	BX,OFFSET OPTONE ; one byte opcodes
	MOV	CX,2*OPBFLN
	CALL	SCANOP
	JNZ	LBL45
	 JMP DO_ONE
LBL45:

	MOV	BX,OFFSET OPTREG ;register
	MOV	CX,2*OPBFLN
	CALL	SCANOP
	JNZ	LBL46
	 JMP DOREG
LBL46:

	MOV	BX,OFFSET OPTBDB ; db and equ
	MOV	CX,2*OPBFLN
	CALL	SCANOP
	JNZ	LBL47
	 JMP DOSIMP
LBL47:

	MOV	BX,OFFSET OPTSMP ; simple
	MOV	CX,2*OPBFLN
	CALL	SCANOP
	JNZ	LBL48
	 JMP DOSIMP
LBL48:

	MOV	BX,OFFSET OPTROT ; rotates
	MOV	CX,2*OPBFLN
	CALL	SCANOP
	JNZ	LBL49
	 JMP DOROT
LBL49:

	MOV	BX,OFFSET OPTDCR ; dcr, inr
	MOV	CX,2*OPBFLN
	CALL	SCANOP
	JNZ	LBL50
	 JMP DODCR
LBL50:

	MOV	BX,OFFSET OPTWRD ;16 bit dcx, inx
	MOV	CX,2*OPBFLN
	CALL	SCANOP
	JNZ	LBL51
	 JMP DODCX
LBL51:

	MOV	BX,OFFSET OPTTDL ;tdl
	MOV	CX,OPBFLN
	CALL	SCANOP
	JNZ	LBL52
	 CALL DOTDL
LBL52:

	MOV	BX,OFFSET OPTRCC ;ret cond
	MOV	CX,2*OPBFLN
	CALL	SCANOP
	JNZ	LBL53
RET
LBL53:

	MOV	BX,OFFSET OPTCCC ;call cond
	MOV	CX,2*OPBFLN
	CALL	SCANOP
	JNZ	LBL54
	 JMP DOCALL
LBL54:

	MOV	BX,OFFSET OPTJCC ;jump cond
	MOV	CX,2*OPBFLN
	CALL	SCANOP
	JNZ	LBL55
	 JMP DOJMP
LBL55:

	MOV	BX,OFFSET OPTMSC ;index & misc
	MOV	CX,2*OPBFLN+2
	CALL	SCANOP
	JNZ	LBL56
	 JMP EXEC
LBL56:

PUTCOD:	MOV	BX,xOPCOD	;this fix prevents macro
	JMP	PUTEND		;names from being split

PUTOPR:	MOV	BX,xOPRND

PUTEND:	XOR	AL,AL
	MOV	LCFLAG,AL
	MOV	LCDFLG,AL

	MOV	CL,N00		;  putout w/o
PUTND1:	MOV	AL,[BX]		;  change

	CMP	AL,' '
	JNZ	LBL57
	 JMP PUTND3
LBL57:
	CMP	AL,HT
	JNZ	LBL58
	 JMP PUTND3
LBL58:

	CMP	AL,CR
	JNZ	LBL59
	 JMP PUTLNC
LBL59:

	CMP	AL,';'
	JNZ	LBL60
	 JMP PUTND4
LBL60:

	CMP	AL,EOS		; process exclamation pt.
	JNZ	LBL61
	 JMP PTND21 ;  statement separator
LBL61:

	CMP	AL,QUOTE
	JZ	LBL62
	 JMP PUTND2
LBL62:
	DEC	CL
	JNZ	LBL63
	 JMP PUTND2
LBL63:
	MOV	CL,N01
PUTND2:	CALL	PUTCHR
	INC	BX
	JMP	PUTND1

;
PTND21:	CALL	SKSPHT
	INC	BX		; Inc past excl. pt.
PTND22:	MOV	AL,[BX]
	CMP	AL,';'
	JNZ	LBL64
	 JMP PUTND5
LBL64:

	MOV	AL,MLTLFL	; Put out as separate
	OR	AL,AL		;  lines
	JNZ	LBL65
	 JMP PTND24 ; NO
LBL65:
;
	MOV	AL,[BX]
	CMP	AL,' '		; Change space to HT
	JZ	LBL66
	 JMP PTND23
LBL66:
	MOV	BYTE PTR [BX],HT
PTND23:	CALL	PCRLF
	JMP	PROCLN0
;
PTND24:	MOV	AL,TEMP		; Was last character put
	CMP	AL,' '		;  out a space ?
	JNZ	LBL67
	 JMP PTND25
LBL67:
	CMP	AL,HT		;  or a TAB ?
	JNZ	LBL68
	 JMP PTND25
LBL68:
	MOV	AL,' '		; NO, put out a space
	CALL	PUTCHR
PTND25:	MOV	AL,'!'
	CALL	PUTCHR
	JMP	PROCLN0
;
;
PUTND3:	PUSH	BX		;Space or Tab come here
	CALL	SKSPHT

	CMP	AL,CR		;this fix filters out
	JNZ	LBL69
	 JMP PUTLNB ;trailing spaces or tabs
LBL69:
	POP	BX

	CMP	AL,EOS		; fix to process excl. pt.
	JNZ	LBL70
	 JMP PTND21
LBL70:

	CMP	AL,';'
	MOV	AL,[BX] ;prevent blank being replaced
				;by ';' in string data
	JNZ	LBL71
	 JMP PUTND4
LBL71:
	CALL	PUTSPT
	JMP	PUTND1
;
PUTND4:	DEC	CL		;';' come here
	INC	CL
	JZ	LBL72
	 JMP PUTND2
LBL72:
	CALL	SKSPHT

; Tab comments to proper column

PUTND5:	MOV	CH,41
PTND51:	MOV	AL,COLNUM
	CMP	AL,CH		;colnum>=41?
	JC	LBL73
	 JMP PTND54
LBL73:
	DEC	AL		;no, insert
	AND	AL,NF8		;tabs to
	ADD	AL,N09		;start output
	CMP	AL,CH		;at col. 33
	JNZ	LBL74
	 JMP PTND54
LBL74:
	JNC	LBL75
	 JMP PTND52
LBL75:
	MOV	AL,' '
	JMP	PTND53
;
PTND52:	MOV	AL,HT
PTND53:	CALL	PUTCHR
	JMP	PTND51

PTND54:	MOV	AL,TEMP		; insure
	CMP	AL,' '		; space
	JNZ	LBL76
	 JMP PUTND6 ; before
LBL76:
	CMP	AL,HT		; semi-colon
	JNZ	LBL77
	 JMP PUTND6 ;
LBL77:
	MOV	AL,' '		;
	CALL	PUTCHR		;
PUTND6:	MOV	AL,[BX]
	INC	BX
	CMP	AL,EOS
	JNZ	LBL78
	 JMP PTND22
LBL78:
	OR	AL,AL
	JNZ	LBL79
RET
LBL79:
	CALL	PUTCHR
	JMP	PUTND6

;
;	Put line at HL to output file until 0
;	and reset colnum to 1.
;
PUTLNB:	XCHG SP,BP 
	XCHG [BP],BX 
	XCHG SP,BP ;filter trailing
	POP	BX		;blanks or tabs

PUTLNC:	JMP	PUTLIN

;*******************************************************
;
;	Process labels, find potential opcode.
;
FNDOPC:	MOV	COLNFL,0	; reset colon flag

	MOV	AL,[BX]
	CMP	AL,' '
	JNZ	LBL80
	 JMP FNDOP3
LBL80:
	CMP	AL,HT
	JNZ	LBL81
	 JMP FNDOP3
LBL81:

	CMP	AL,CR		;pass blank
	JNZ	LBL82
	 RET	;lines
LBL82:
	CMP	AL,EOS		; excl. pt. separator
	JNZ	LBL83
RET
LBL83:

	CMP	AL,';'
	JNZ	LBL84
RET
LBL84:

	CMP	AL,'*'		; asterisk in first column
	JZ	LBL85
	 JMP FNDOP1 ; is a comment line
LBL85:
	MOV	BYTE PTR [BX],';'
	RET

FNDOP1:	MOV	CL,N00
	MOV	COLNFL,':'	; Set colon flag to
				;  insure colon after label
FNDOP2:	MOV	AL,BYTE PTR [BX]

	CMP	AL,':'
	JNZ	LBL86
	 JMP FNDOP4
LBL86:

	CMP	AL,HT
	JNZ	LBL87
	 JMP FNDOP6
LBL87:
	CMP	AL,' '
	JNZ	LBL88
	 JMP FNDOP6
LBL88:

	CMP	AL,CR
	JNZ	LBL89
RET
LBL89:
	CMP	AL,EOS
	JNZ	LBL90
RET
LBL90:

	CMP	AL,';'
	JNZ	LBL91
	 JMP FNDP72
LBL91:

	CALL	PUTCHR
	INC	BX
	INC	CL
	JMP	FNDOP2

; Comes here only if space or tab at beginning
; of line.

FNDOP3:
	PUSH	BX
	CALL	SKSPHT		;find first non-sp or tab
	CMP	AL,CR
	JNZ	LBL92
	 JMP FNDOP9
LBL92:
	CMP	AL,EOS
	JNZ	LBL93
	 JMP FNDOP9
LBL93:
	CMP	AL,';'
	JNZ	LBL94
	 JMP FNDP71
LBL94:
	POP	BX

	CALL	PUTSPT		;print until non-sp or ht
	PUSH	BX
	CALL	FINDLM		;find ,:+-/*); CR HT or SP at HL
	CMP	AL,':'
	POP	BX
	JNZ	LBL95
	 JMP FNDOP1
LBL95:
	JMP	FNDOP7
;
; Colon terminating label comes here
;
FNDOP4:	INC	BX
	MOV	AL,[BX]
	CMP	AL,':'
	JZ	LBL96
	 JMP FNDOP5
LBL96:
	CALL	PUTCHR
	INC	BX
FNDOP5:	MOV	COLNFL,':'	; Set colon flag
;
; HT or SP comes here
;
FNDOP6:
;
; See if there is an opcode field
;
FNDOP7:	PUSH	BX

	CALL	SKSPHT
	MOV	AL,[BX]

	CMP	AL,CR
	JNZ	LBL97
	 JMP FNDOP9 ; filter trailing SP or TAB
LBL97:
	CMP	AL,EOS
	JNZ	LBL98
	 JMP FNDOP9 ; excl. pt. separator
LBL98:

	CMP	AL,';'
	JZ	LBL99
	 JMP FNDOP8
LBL99:

FNDP71:	XCHG SP,BP 
	 XCHG [BP],BX 
	XCHG SP,BP
	POP	BX
FNDP72:	POP	CX		; clear return
	CALL	PTCOLN		; put out colon if flag set
	JMP	PUTND5		; tab to proper column
;
; Have located opcode field
;
FNDOP8:	POP	BX
	MOV	xWHITE,BX
	CALL	SKSPHT

;	Move potential opcode to OPCBUF

MOVOPC:	MOV	xOPCOD,BX
	MOV	CH,OPBFLN
	MOV	DX,OFFSET OPCBUF
	CALL	MOVBDH		;move up to B char from HL to
	CALL	SKSPHT		;DE until ,:+-/*); CR HT SP
	MOV	xOPRND,BX
	SUB	AL,AL
	INC	AL
	RET

; come here on CR to filter trailing SP or TAB

FNDOP9:	CALL	PTCOLN		; put out colon if flag set
	XCHG SP,BP 
	 XCHG [BP],BX 
	XCHG SP,BP
	POP	BX
	XOR	AL,AL
	RET
;
;	Put out colon if COLNFL set.
;
PTCOLN:	MOV	AL,COLNFL
	OR	AL,AL
	JZ	PTCLN1
	CALL	PUTCHR
	MOV	COLNFL,0
PTCLN1:	RET
;
COLNFL	DB	0
xWHITE	DW	1 DUP(?)

;*******************************************************
;
;	Opcode tables
;
OPTIMM	DB	'ACI  ADC  '
	DB	'ADI  ADD  '
	DB	'ANI  AND  '
	DB	'CPI  CMP  '
	DB	'ORI  OR   '
	DB	'SBI  SBB  '
	DB	'SUI  SUB  '
	DB	'XRI  XOR  '
	DB	0

OPTONE	DB	'RET  RET  '
	DB	'CMC  CMC  '
	DB	'HLT  HLT  '
	DB	'STC  STC  '
	DB	'DAA  DAA  '
	DB	'DI   CLI  '
	DB	'EI   STI  '
	DB	'NOP  NOP  '
	DB	0

OPTREG	DB	'ADC  ADC  '
	DB	'ADD  ADD  '
	DB	'ANA  AND  '
	DB	'CMP  CMP  '
	DB	'ORA  OR   '
	DB	'SBB  SBB  '
	DB	'SUB  SUB  '
	DB	'XRA  XOR  '
	DB	0

OPTPSD	DB	'DB   '
	DB	'DW   '
	DB	'EQU  '
	DB	'DS   '
	DB	0

OPTBDB	DB	'DB   DB   '
	DB	'EQU  EQU  '
	DB	0

OPTSMP	DB	'JMP  JMP  '
	DB	'CALL CALL '
	DB	'DS   RS   '
	DB	'DW   DW   '
	DB	'SET  EQU  '
OPTENT	DB	'ENT  ENTRY'
	DB	'NAM  NAME '
	DB	'RAM  DATA '
	DB	'ROG  REL  '
	DB	0

OPTDCR	DB	'DCR  DEC  '
	DB	'INR  INC  '
	DB	0

OPTROT	DB	'RAL  RCL  '
	DB	'RAR  RCR  '
	DB	'RLC  ROL  '
	DB	'RRC  ROR  '
	DB	0

OPTWRD	DB	'DCX  DEC  '
	DB	'INX  INC  '
	DB	0

OPTTDL	DB	'REQ  '
	DB	'RNE  '
	DB	'RLT  '
	DB	'RGE  '
	DB	'CEQ  '
	DB	'CNE  '
	DB	'CLT  '
	DB	'CGE  '
	DB	'JEQ  '
	DB	'JNE  '
	DB	'JLT  '
	DB	'JGE  '
	DB	0

OPTRCC	DB	'RC   JNC  '
	DB	'RNC  JC   '
	DB	'RZ   JNZ  '
	DB	'RNZ  JZ   '
	DB	'RP   JS   '
	DB	'RM   JNS  '
	DB	'RPE  JPO  '
	DB	'RPO  JPE  '
	DB	0

OPTCCC	DB	'CC   JNC  '
	DB	'CNC  JC   '
	DB	'CZ   JNZ  '
	DB	'CNZ  JZ   '
	DB	'CP   JS   '
	DB	'CM   JNS  '
	DB	'CPE  JPO  '
	DB	'CPO  JPE  '
	DB	0

OPTJCC	DB	'JC   JNC  '
	DB	'JNC  JC   '
	DB	'JZ   JNZ  '
	DB	'JNZ  JZ   '
	DB	'JP   JS   '
	DB	'JM   JNS  '
	DB	'JPE  JPO  '
	DB	'JPO  JPE  '
	DB	0

OPTMSC	DB	'LXI  MOV  '
	DW	DOLXI
	DB	'POP  POP  '
	DW	DOPOP
	DB	'PUSH PUSH '
	DW	DOPSH
	DB	'DAD  ADD  '
	DW	DODAD
	DB	'LDA  MOV  '
	DW	DOLDA
	DB	'LDAX MOV  '
	DW	DOLDAX
	DB	'LHLD MOV  '
	DW	DOLHLD
	DB	'MOV  MOV  '
	DW	DOMOV
	DB	'MVI  MOV  '
	DW	DOMVI
	DB	'IN   IN   '
	DW	DOIN
	DB	'OUT  OUT  '
	DW	DOOUT
	DB	'PCHL JMP  '
	DW	DOPCHL
	DB	'RST  CALL '
	DW	DORST
	DB	'SHLD MOV  '
	DW	DOSHLD
	DB	'SPHL MOV  '
	DW	DOSPHL
	DB	'STA  MOV  '
	DW	DOSTA
	DB	'STAX MOV  '
	DW	DOSTAX
	DB	'XCHG XCHG '
	DW	DOXCHG
	DB	'XTHL XCHG '
	DW	DOXTHL
	DB	'CMA  NOT  '
	DW	DOCMA
	DB	'IF   IF   '
	DW	DOIFC
	DB	'LIST LIST '
	DW	DOLST
	DB	'MACROMACRO'
	DW	DOMAC
OPTIFC	DB	'IFC  IF   '
	DW	DOIFC
	DB	'ICL  *INCL'
	DW	DOICL
	DB	'LST  LIST '
	DW	DOLST
	DB	'MAC  MACRO'
	DW	DOMAC
	DB	0

;*******************************************************
;
;	Scan table at HL for match to OPBFLN
;	char string at OPCBUF.
;	Ret Z and BX -> entry if match.
;
SCANOP:	MOV	AL,[BX]
	AND	AL,AL
	JNZ	LBL100
	 JMP SCNOP1
LBL100:
	PUSH	CX
	MOV	CH,OPBFLN
	MOV	DX,OFFSET OPCBUF
	CALL	CBDEHL		;comp B bytes (DE)-(HL)
	POP	CX
	JNZ	LBL101
RET
LBL101:
	ADD	BX,CX
	JMP	SCANOP
;
SCNOP1:	INC	AL
	RET

;
;	Gets routine address  from
;	HL+2*OPBFLN and jumps to routine.
;
EXEC:	PUSH	BX
	MOV	CX,2*OPBFLN
	ADD	BX,CX
	MOV	CL,[BX]
	INC	BX
	MOV	CH,[BX]
	POP	BX
	PUSH	CX		;address on stack
	RET			;go to it

;
;	Put up to OPBFLN char at HL+OPBFLN
;	to output file.  Stop at space,
;	and put tab to output file.
;
PUTOPHT:
	CALL	PUTOPC
PUTHT:	MOV	AL,HT
	JMP	PUTCHR

;
;	Put space or tab (contents of MLTSPC)
;	to output file to separate opcode from
;	operand in statements that get translated
;	to multiple statements.
;
PUTHTS:	MOV	AL,MLTSPC
	JMP	PUTCHR
;
MLTSPC	DB	HT

;
;
;
PUTOPC:	MOV	CX,OFFSET OPBFLN
	ADD	BX,CX		; HL -> new opcode
	MOV	CH,CL
PUTOP1:	MOV	AL,[BX]

	CMP	AL,' '
	JNZ	LBL102
RET
LBL102:

	CMP	AL,HT
	JNZ	LBL103
RET
LBL103:

	MOV	AL,LCFLAG
	OR	AL,AL
	MOV	AL,[BX]
	JNZ	LBL104
	 JMP PUTOP2
LBL104:
	OR	AL,20H
PUTOP2:
	CALL	PUTCHR
	INC	BX
	DEC	CH
	JZ	LBL105
	 JMP PUTOP1
LBL105:

	RET

;
;	Put string at HL to output file until 0.
;	If (LCFLAG) set, convert to lower case.
;
PUTOPS:	MOV	AL,[BX]
	OR	AL,AL
	JNZ	LBL106
RET
LBL106:
	MOV	AL,LCFLAG
	OR	AL,AL
	MOV	AL,[BX]
	JNZ	LBL107
	 JMP PUTOS0
LBL107:
	CALL	LCASE
PUTOS0:	CALL	PUTCHR
	INC	BX
	JMP	PUTOPS

;
;	Put string at HL to output file until 0.
;	If (LCDFLG) set, convert to lower case.
;
PUTRND:	MOV	AL,[BX]
	OR	AL,AL
	JNZ	LBL108
RET
LBL108:
	MOV	AL,LCDFLG
	OR	AL,AL
	MOV	AL,[BX]
	JNZ	LBL109
	 JMP PUTRN0
LBL109:
	CALL	LCASE
PUTRN0:	CALL	PUTCHR
	INC	BX
	JMP	PUTRND
;
LCDFLG	DB	0

;
;	Find first ,:+-/*); CR HT or SP at HL,
;	return A = (HL).
;
FINDLM:	PUSH	CX
	CALL	CHKDLM
	POP	CX
	JNZ	LBL110
RET
LBL110:
	INC	BX
	JMP	FINDLM
;
;	Fill B locations at DE with spaces.
;	Move up to B char from HL to DE until
;	,:+-/*); CR HT or SP encountered.
;	Return Z and HL->special char if found.
;	(search B+1 loc for special char.)
;
MOVBDH:	MOV	CL,CH
	MOV	CH,N00
	PUSH	CX
	PUSH	DX
	PUSH	BX		; fill BC locations
	CALL	FILLBD		; at DE with spaces
	POP	BX		;
	POP	DX
	POP	CX
MOVBD1:	PUSH	CX		; ret Z, A=(HL)
	CALL	CHKDLM		; if (HL) is
	POP	CX		; ,:+-/*); CR HT or SP
	JNZ	LBL111
RET
LBL111:

	MOV	AL,[BX]
	XCHG BX,DX 
	 MOV [BX],AL 
	XCHG BX,DX
	INC	DX
	INC	BX
	DEC	CX
	MOV	AL,CH
	OR	AL,CL
	JNZ	LBL112
	 JMP CHKDLM
LBL112:
	JMP	MOVBD1

;
;	Skip spaces and tabs.
;	Return HL -> non-space or non-tab
;
SKSPHT:	MOV	AL,[BX]
	CMP	AL,' '
	JNZ	LBL113
	 JMP SKSPT1
LBL113:
	CMP	AL,HT
	JZ	LBL114
RET
LBL114:
SKSPT1:	INC	BX
	JMP	SKSPHT

;
;	Ret Z, A=(HL) if
;	HL is ,:+-/*); CR HT SP or EOS
;
CHKDLM:	MOV	AL,[BX]
	CMP	AL,HT
	JZ	CHKDRZ
	CMP	AL,' '
	JZ	CHKDRZ
	CMP	AL,','
	JZ	CHKDRZ
	CMP	AL,';'
	JZ	CHKDRZ
	CMP	AL,CR
	JZ	CHKDRZ
	CMP	AL,':'
	JZ	CHKDRZ
	CMP	AL,'+'
	JZ	CHKDRZ
	CMP	AL,'-'
	JZ	CHKDRZ
	CMP	AL,'/'
	JZ	CHKDRZ
	CMP	AL,'*'
	JZ	CHKDRZ
	CMP	AL,')'
	JZ	CHKDRZ
	CMP	AL,EOS
CHKDRZ:	RET
;
;	Compares B chars at DE with chars
;	at HL.
;	Ret Z if match.
;	Preserve HL, DE, BC
;
CBDEHL:	PUSH	BX
	PUSH	DX
	PUSH	CX
CBDH1:	XCHG BX,DX 
	 MOV AL,[BX] 
	XCHG BX,DX

	CMP	AL,'a'
	JNC	LBL115
	 JMP CBDH2
LBL115:
	MOV	LCFLAG,AL
	AND	AL,05FH
CBDH2:
	CMP	AL,[BX]
	JZ	LBL116
	 JMP CBDH3
LBL116:
	INC	BX
	INC	DX
	DEC	CH
	JZ	LBL117
	 JMP CBDH1
LBL117:
CBDH3:	POP	CX
	POP	DX
	POP	BX
	RET

LCFLAG	DB	0

;
;	Fill BC locations starting at
;	DE with spaces.
;	Returns A = space, DE -> next free
;	location, HL = DE - 1, BC = 0.
;
FILLBD:	MOV	AL,' '
	XCHG BX,DX 
	 MOV [BX],AL 
	XCHG BX,DX
	MOV	BH,DH
	MOV	BL,DL
	INC	DX
	DEC	CX
	CALL	MOVIR
	RET

;
;	(DE)=(HL), INC HL, INC DE, DEC BC
;	Repeat until BC = 0.
;
MOVIR:	MOV	AL,[BX]
	XCHG BX,DX 
	 MOV [BX],AL 
	XCHG BX,DX
	INC	BX
	INC	DX
	DEC	CX
	MOV	AL,CH
	OR	AL,CL
	JZ	LBL118
	 JMP MOVIR
LBL118:
	RET

;*******************************************************
;
;	Translation Routines
;
;*******************************************************

;
;	Immediate
;	e.g., XRI n -> XOR AL,n
;
DOIMM:	CALL	PUTOPHT
	MOV	BX,OFFSET OPALC
	CALL	PUTOPS
	JMP	PUTOPR
;
OPALC	DB	'AL,',0

;
;	One byte (implied)
;	e.g., DI -> CLI
;
DO_ONE:	CALL	PUTOPC
	JMP	PUTOPR

;
; Simple translation
;	e.g., DS n -> RS n
;
DOSIMP:	CALL	PUTOPHT
	JMP	PUTOPR

;
;	Register instructions
;	e.g., XRA r -> XOR AL,r'
;
DOREG:	CALL	PUTOPHT		;put out opcode+tab
	MOV	BX,OFFSET OPALC
	CALL	PUTOPS
	MOV	BX,xOPRND
	CALL	TRNRG
	JNZ	DOREG1
	XCHG	DX,BX
	CALL	PUTRND
	XCHG	DX,BX
DOREG1:	JMP	PUTEND

;
;	MOV r,s -> MOV r',s'
;
DOMOV:	CALL	PUTOPHT
	MOV	BX,xOPRND
	CALL	TRNRG
	JZ	LBL119
	 JMP PUTOPR
LBL119:

	XCHG	DX,BX
	CALL	PUTRND
	XCHG	DX,BX

	MOV	AL,[BX]	; Get comma
	INC	BX	; Increment past comma
	CALL	PUTCHR
	CALL	TRNRG
	JNZ	DOMOV1

	XCHG	DX,BX
	CALL	PUTRND
	XCHG	DX,BX

DOMOV1:	JMP	PUTEND

;
; Dec and Inc byte register
;	DCR r -> DEC r'
;
DODCR:

;
;	MVI r,n -> MOV r',n
;
DOMVI:	CALL	PUTOPHT

	MOV	BX,xOPRND
	CALL	TRNRG
	JNZ	DOMVI2

	XCHG	BX,DX
	MOV	AL,[BX]
	XCHG	BX,DX
	CMP	AL,'['
	JNZ	DOMVI1

	PUSH	BX
	MOV	BX,OFFSET OPBYTP
	CALL	PUTLIN
	POP	BX

DOMVI1:	XCHG	BX,DX
	CALL	PUTRND
	XCHG	BX,DX
DOMVI2:	JMP	PUTEND
;
OPBYTP	DB	'BYTE PTR ',0

;
; Translate 8080 byte registers to
; 8086 byte registers.
; Enter with BX -> to 8080 register.
; If match,
;  return Z set, BX -> next, DX -> translation
; Otherwise, return NZ, BX, DX unchanged.
;
TRNRG:	MOV	AL,[BX]
	CMP	AL,'a'
	JNC	LBL120
	 JMP TRNRG2
LBL120:
	MOV	LCDFLG,AL
TRNRG2:	AND	AL,5FH
	PUSH	BX
	MOV	BX,OFFSET RTBL
	MOV	CX,OFFSET RTBLE
	SUB	CX,BX
	MOV	CH,CL
TRNRG3:	CMP	AL,[BX]
	JNZ	LBL121
	 JMP TRNRG4
LBL121:
	INC	BX
	DEC	CH
	JZ	LBL122
	 JMP TRNRG3
LBL122:
	POP	BX		; HL -> R
	MOV	AL,0FFH		; return NZ
	OR	AL,AL		; if no match
	RET
;
TRNRG4:	MOV	DX,OFFSET RTBL
	SUB	BX,DX
	ADD	BX,BX
	MOV	DX,OFFSET RPTBL
	ADD	BX,DX
	MOV	DL,[BX]
	INC	BX
	MOV	DH,[BX]
	POP	BX
	INC	BX
	XOR	AL,AL
	RET
;
RTBL	DB	'ABCDEHLM'
RTBLE:
RPTBL	DW	ALREG
	DW	CHREG
	DW	CLREG
	DW	DHREG
	DW	DLREG
	DW	BHREG
	DW	BLREG
	DW	PBX
;
ALREG	DB	'AL',0
CHREG	DB	'CH',0
CLREG	DB	'CL',0
DHREG	DB	'DH',0
DLREG	DB	'DL',0
BHREG	DB	'BH',0
BLREG	DB	'BL',0
PBX	DB	'[BX]',0

;
; Rotates
;
DOROT:	CALL	PUTOPHT
	MOV	BX,OFFSET OPALC
	CALL	PUTOPS
	MOV	AL,'1'
	CALL	PUTCHR
	JMP	PUTOPR

;
;	DAD rp -> ADD BX,rp'
;
DODAD:	MOV	AL,DADFL
	OR	AL,AL
	JZ	DODAD2

	PUSH	BX
	MOV	BX,OFFSET OPPSHF ; 'PUSHF'
	CALL	PUTOPS
	CALL	SEP	; Put out separator
	POP	BX

	CALL	PUTOPC
	CALL	PUTHTS

DODAD1:	MOV	BX,OFFSET OPBXC	;'BX,'
	CALL	PUTOPS

	MOV	BX,xOPRND
	CALL	TRNRP	; DX -> translated rp
	JZ	DODAD3
	JMP	PUTOPR

DODAD2:	CALL	PUTOPHT
	JMP	DODAD1

DODAD3:	XCHG	BX,DX
	MOV	AL,[BX]
	XCHG	BX,DX
	CMP	AL,'A'
	JNZ	LBL123
	JMP	PUTOPR
LBL123:

	XCHG	BX,DX	; BX -> translated rp
	CALL	PUTRND
	XCHG	BX,DX	; BX -> next

	MOV	AL,DADFL
	OR	AL,AL
	JZ	DODAD4

	PUSH	BX
	CALL	SEP	; Put out separator
	MOV	BX,OFFSET OPPOPF ; 'POPF'
	CALL	PUTOPS
	POP	BX

DODAD4:	JMP	PUTEND
;
OPBXC	DB	'BX,',0

;
;	DCX or INX rp -> DEC or INC rp'
;
DODCX:	MOV	AL,INXFL
	OR	AL,AL
	JZ	DODCX2

	PUSH	BX
	MOV	BX,OFFSET OPPSHF ; 'PUSHF'
	CALL	PUTOPS
	CALL	SEP	; Put out separator
	POP	BX

	CALL	PUTOPC
	CALL	PUTHTS

DODCX1:	MOV	BX,xOPRND
	CALL	TRNRP	; DX -> translated rp
	JZ	DODCX3
	JMP	PUTOPR

DODCX2:	CALL	PUTOPHT
	JMP	DODCX1

DODCX3:	XCHG	BX,DX
	MOV	AL,[BX]
	XCHG	BX,DX
	CMP	AL,'A'
	JNZ	LBL124
	JMP	PUTOPR
LBL124:

	XCHG	BX,DX	; BX -> translated rp
	CALL	PUTRND
	XCHG	BX,DX	; BX -> next

	MOV	AL,INXFL
	OR	AL,AL
	JZ	DODCX4

	PUSH	BX
	CALL	SEP	; Put out separator
	MOV	BX,OFFSET OPPOPF ; 'POPF'
	CALL	PUTOPS
	POP	BX

DODCX4:	JMP	PUTEND
;
OPPSHF	DB	'PUSHF',0
OPPOPF	DB	'POPF',0

;
; PUSH rp -> PUSH rp'
;
DOPSH:	XCHG	BX,DX
	MOV	BX,xOPRND
	MOV	AL,[BX]
	AND	AL,5FH
	CMP	AL,'P'
	XCHG	BX,DX
	JNZ	DOPSH1
	XCHG	BX,DX
	CALL	TRNRP		; DE -> trans, HL -> next
	JZ	LBL125
	 JMP PUTCOD
LBL125:
	PUSH	BX
	MOV	BX,OFFSET OPLAHF
	CALL	PUTRND

	MOV	AL,PSWFL	; Preserve order of
	OR	AL,AL		;  registers on stack ?
	JZ	LBL126
	CALL	XAHAL		; Yes, XCHG AH,AL
LBL126:

	CALL	SEP		; SP,EP,SP or CR, LF, HT
	MOV	BX,OFFSET OPPUSH
	CALL	PUTOPS
	CALL	PUTHTS
	POP	BX
	XCHG	BX,DX		; BX -> translated rp
	CALL	PUTRND
	XCHG	BX,DX		; BX -> next

	MOV	AL,PSWFL
	OR	AL,AL
	JZ	LBL127
	CALL	XAHAL
LBL127:

	JMP	PUTEND
;
DOPSH1:	CALL	PUTOPHT
DOPSH2:	MOV	BX,xOPRND
	CALL	TRNRP		; DX -> translated rp
	JZ	LBL128
	JMP	PUTOPR
LBL128:

DOPSH3:	XCHG	BX,DX		; BX -> translated rp
	CALL	PUTRND
	XCHG	BX,DX		; BX -> next
	JMP	PUTEND
;
OPLAHF	DB	'LAHF',0
OPPUSH	DB	'PUSH',0

;
XAHAL:	PUSH	BX
	CALL	SEP
	MOV	BX,OFFSET OPXCHG
	CALL	PUTRND
	CALL	PUTHTS
	MOV	BX,OFFSET OPAHAL
	CALL	PUTRND
	POP	BX
	RET
;
OPXCHG	DB	'XCHG',0
OPAHAL	DB	'AH,AL',0

;
; POP rp -> POP rp'
;
DOPOP:	XCHG	BX,DX		; save BX in DX
	MOV	BX,xOPRND
	MOV	AL,[BX]
	AND	AL,5FH
	CMP	AL,'P'		; Is it PSW ?
	XCHG	BX,DX		; restore BX
	JNZ	DOPSH1

	CALL	PUTOPC
	MOV	AL,PSWFL
	OR	AL,AL
	JZ	DOPOP1
	CALL	PUTHTS		; Put space or tab
	JMP	DOPOP2

DOPOP1:	CALL	PUTHT
DOPOP2:	MOV	BX,xOPRND
	CALL	TRNRP		; DE -> trans, HL -> next
	JZ	LBL129
	 JMP PUTOPR
LBL129:

	XCHG	BX,DX		; BX -> tranlated rp
	CALL	PUTRND

	MOV	AL,PSWFL
	OR	AL,AL
	JZ	LBL130
	CALL	XAHAL
LBL130:

	CALL	SEP		; SP,EP,SP or CR, LF, HT
	MOV	BX,OFFSET OPSAHF
	CALL	PUTRND
	XCHG	BX,DX
	JMP	PUTEND
;
OPSAHF	DB	'SAHF',0

;
; LXI rp,n -> MOV rp',OFFSET n
;
DOLXI:	CALL	PUTOPHT
	MOV	BX,xOPRND
	CALL	TRNRP
	JZ	LBL131
	 JMP PUTOPR
LBL131:
	XCHG BX,DX 
	 MOV AL,[BX] 
	XCHG BX,DX
	CMP	AL,'A'
	JNZ	LBL132
	 JMP PUTOPR
LBL132:
	XCHG	BX,DX
	CALL	PUTRND
	MOV	BX,OFFSET OFFATR
	CALL	PUTLIN
	XCHG	BX,DX		; HL -> next
	INC	BX		; skip comma
	JMP	PUTEND
;
OFFATR	DB	',OFFSET ',0

;
; Translate 16 bit registers.
; Enter with HL -> rp.
; Returns HL -> next char, DE -> translation,
; Z set if match,
; otherwise, HL unchanged, NZ.
;
TRNRP:	XOR	AL,AL
	MOV	LCDFLG,AL
	MOV	AL,[BX]
	CMP	AL,'a'
	JNC	LBL133
	 JMP TRNRP1
LBL133:
	MOV	LCDFLG,AL
TRNRP1:	AND	AL,5FH
	CMP	AL,'B'
	JNZ	LBL134
	 JMP TRNRPB
LBL134:
	CMP	AL,'D'
	JNZ	LBL135
	 JMP TRNRPD
LBL135:
	CMP	AL,'H'
	JNZ	LBL136
	 JMP TRNRPH
LBL136:
	CMP	AL,'P'
	JNZ	LBL137
	 JMP TRNRPP
LBL137:
	CMP	AL,'S'
	JNZ	LBL138
	 JMP TRNRPS
LBL138:
TRNRP2:	MOV	AL,0
	MOV	LCDFLG,AL
	RET

;
TRNRPB:	MOV	DX,OFFSET OPRCX	;'CX'
	INC	BX
	XOR	AL,AL
	RET
;
TRNRPD:	MOV	DX,OFFSET OPRDX	;'DX'
	INC	BX
	XOR	AL,AL
	RET
;
TRNRPH:	MOV	DX,OFFSET OPRBX	;'BX'
	INC	BX
	XOR	AL,AL
	RET
;
TRNRPP:	INC	BX
	MOV	AL,[BX]
	AND	AL,5FH
	CMP	AL,'S'
	JZ	LBL139
	 JMP TRNRP4
LBL139:
	INC	BX
	MOV	AL,[BX]
	AND	AL,5FH
	CMP	AL,'W'
	JZ	LBL140
	 JMP TRNRP3
LBL140:
	MOV	DX,OFFSET OPRAX	;'AX'
	INC	BX
	XOR	AL,AL
	RET
;
TRNRP3:	DEC	BX
TRNRP4:	DEC	BX
	JMP	TRNRP2
;
TRNRPS:	INC	BX
	MOV	AL,[BX]
	AND	AL,5FH
	CMP	AL,'P'
	JZ	LBL141
	 JMP TRNRP4
LBL141:
	MOV	DX,OFFSET OPRSP
	INC	BX
	XOR	AL,AL
	RET
;
OPRAX	DB	'AX',0
OPRCX	DB	'CX',0
OPRDX	DB	'DX',0
OPRBX	DB	'BX',0
OPRSP	DB	'SP',0

;
;	Strange opcodes
;
DOTDL:	MOV	AL,OPCBUF+1

	MOV	BX,OFFSET CCZ	;'Z '
	CMP	AL,'E'
	JNZ	LBL142
	 JMP DOTDL1
LBL142:
	MOV	BX,OFFSET CCNZ	;'NZ'
	CMP	AL,'N'
	JNZ	LBL143
	 JMP DOTDL1
LBL143:
	MOV	BX,OFFSET CCC	;'C '
	CMP	AL,'L'
	JNZ	LBL144
	 JMP DOTDL1
LBL144:
	MOV	BX,OFFSET CCNC	;'NC'

	CMP	AL,'G'
	JNZ	LBL145
	 JMP DOTDL1
LBL145:
	MOV	BX,OFFSET CCZL
	CMP	AL,'e'
	JNZ	LBL146
	 JMP DOTDL1
LBL146:
	MOV	BX,OFFSET CCNZL
	CMP	AL,'n'
	JNZ	LBL147
	 JMP DOTDL1
LBL147:
	MOV	BX,OFFSET CCCL
	CMP	AL,'l'
	JNZ	LBL148
	 JMP DOTDL1
LBL148:
	MOV	BX,OFFSET CCNCL

DOTDL1:	MOV	AL,[BX]
	MOV	OPCBUF+1,AL
	INC	BX
	MOV	AL,[BX]
	MOV	OPCBUF+2,AL
	RET
;
CCZ	DB	'Z '
CCNZ	DB	'NZ'
CCC	DB	'C '
CCNC	DB	'NC'

CCZL	DB	'z '
CCNZL	DB	'nz'
CCCL	DB	'c '
CCNCL	DB	'nc'

;
;	Return conditional
;
DORET:	CALL	PUTOPC
	CALL	PUTHT		; Could change to PUTHTS
	MOV	BX,OFFSET REL3
	CALL	PUTLIN
	CALL	SEP
	MOV	BX,OFFSET OPRET
	CALL	PUTOPS
	JMP	PUTOPR
;
REL3	DB	'$+3',0
OPRET	DB	'RET',0

;
;	Call conditional
;
DOCALL:	CALL	PUTOPC
	CALL	PUTHT		; Could change to PUTHTS
	MOV	BX,OFFSET REL5
	CALL	PUTLIN
	CALL	SEP		; SP,EP,SP or CR, LF, HT
	MOV	BX,OFFSET OPCALL
	CALL	PUTOPS
	CALL	PUTHTS
	JMP	PUTOPR
;
REL5	DB	'$+5',0
OPCALL	DB	'CALL',0

;
;	Jump conditional
;
DOJMP:	CALL	PUTOPC
	CALL	PUTHT		; Could change to PUTHTS
	MOV	BX,OFFSET REL5
	CALL	PUTLIN
	CALL	SEP		; SP,EP,SP or CR, LF, HT
	MOV	BX,OFFSET OPJMP
	CALL	PUTOPS
	CALL	PUTHTS		; Put space or tab
	JMP	PUTOPR
;
OPJMP	DB	'JMP',0

;
;	IN n -> IN AL,n
;
DOIN:	JMP	DOIMM

;
;	LDA addr -> MOV AL,addr
;
DOLDA:	JMP	DOIMM

;
;		   XCHG BX,rp'
;	LDAX rp -> MOV AL,[BX]
;		   XCHG BX,rp'
;
DOLDAX:	MOV	BX,xOPRND
	CALL	TRNRP		; DE -> trans, HL -> next
	JZ	LBL151
	 JMP PUTCOD
LBL151:

	PUSH	BX
	MOV	BX,OFFSET OPXCHG
	CALL	PUTOPS
	CALL	PUTHTS
	MOV	BX,OFFSET OPBXC
	CALL	PUTOPS
	XCHG	BX,DX
	PUSH	BX
	CALL	PUTRND
	CALL	SEP		; SP,EP,SP or CR, LF, HT

	MOV	BX,OFFSET OPMOV
	CALL	PUTOPS
	CALL	PUTHTS
	MOV	BX,OFFSET OPLDAX
	CALL	PUTOPS
	CALL	SEP		; SP,EP,SP or CR, LF, HT

	MOV	BX,OFFSET OPXCHG
	CALL	PUTOPS
	CALL	PUTHTS
	MOV	BX,OFFSET OPBXC
	CALL	PUTOPS
	POP	BX		; HL -> rp'
	CALL	PUTRND

	POP	BX
	JMP	PUTEND
;
OPMOV	DB	'MOV',0
OPLDAX	DB	'AL,[BX]',0

;
;	LHLD addr -> MOV BX,addr
;
DOLHLD:	CALL	PUTOPHT
	MOV	BX,OFFSET OPBXC	; 'BX,'
	CALL	PUTOPS
	JMP	PUTOPR

;
;	OUT n -> OUT n,AL
;
DOOUT:	CALL	PUTOPHT
	CALL	PUTEXP
	PUSH	BX
	MOV	BX,OFFSET OPCAL	; ',AL'
	JMP	DOSTA1

;
;	PCHL -> JMP BX
;
DOPCHL:	CALL	PUTOPHT
	MOV	BX,OFFSET OPRBX	; 'BX'
	CALL	PUTOPS
	JMP	PUTOPR

;
;	RST -> CALL 8*
;
DORST:	CALL	PUTOPHT
	MOV	BX,OFFSET OPR8M	;'8*'
	CALL	PUTOPS
	JMP	PUTOPR
;
OPR8M	DB	'8*',0

;
;	SHLD addr -> MOV addr,BX
;
DOSHLD:	CALL	PUTOPHT
	CALL	PUTEXP
	PUSH	BX
	MOV	BX,OFFSET OPCBX	; ',BX'
	JMP	DOSTA1
;
OPCBX	DB	',BX',0

;
;	SPHL -> MOV SP,BX
;
DOSPHL:	CALL	PUTOPHT
	MOV	BX,OFFSET OPSPBX ; 'SP,BX'
	CALL	PUTOPS
	JMP	PUTOPR
;
OPSPBX	DB	'SP,BX',0

;
;	STA addr -> MOV addr,AL
;
DOSTA:	CALL	PUTOPHT
	CALL	PUTEXP
	PUSH	BX
	MOV	BX,OFFSET OPCAL	; ',AL'
DOSTA1:	CALL	PUTOPS
	POP	BX
	JMP	PUTEND
;
OPCAL	DB	',AL',0

;
;		   XCHG BX,rp'
;	STAX rp -> MOV [BX],AL
;		   XCHG BX,rp'
;
DOSTAX:	MOV	BX,xOPRND
	CALL	TRNRP		; DE -> trans, HL -> next
	JZ	LBL152
	 JMP PUTCOD
LBL152:

	PUSH	BX
	MOV	BX,OFFSET OPXCHG
	CALL	PUTOPS
	CALL	PUTHTS
	MOV	BX,OFFSET OPBXC
	CALL	PUTOPS
	XCHG	BX,DX
	PUSH	BX
	CALL	PUTRND
	CALL	SEP		; SP,EP,SP or CR, LF, HT

	MOV	BX,OFFSET OPMOV
	CALL	PUTOPS
	CALL	PUTHTS
	MOV	BX,OFFSET OPSTAX
	CALL	PUTOPS
	CALL	SEP		; SP,EP,SP or CR, LF, HT

	MOV	BX,OFFSET OPXCHG
	CALL	PUTOPS
	CALL	PUTHTS
	MOV	BX,OFFSET OPBXC
	CALL	PUTOPS
	POP	BX		; HL -> rp'
	CALL	PUTRND

	POP	BX
	JMP	PUTEND
;
OPSTAX	DB	'[BX],AL',0

;
;	XCHG -> XCHG BX,DX
;
DOXCHG:	CALL	PUTOPHT
	MOV	BX,OFFSET OPBXDX ; 'BX,DX'
	CALL	PUTOPS
	JMP	PUTOPR
;
OPBXDX	DB	'BX,DX',0

;
;		XCHG SP,BP
;	XTHL -> XCHG [BP],BX
;		XCHG SP,BP
;
DOXTHL:	MOV	BX,OFFSET OPXCHG
	CALL	PUTOPS
	CALL	PUTHTS
	MOV	BX,OFFSET OPSPBP ; 'SP,BP'
	CALL	PUTOPS
	CALL	SEP		; SP,EP,SP or CR, LF, HT

	MOV	BX,OFFSET OPXCHG
	CALL	PUTOPS
	CALL	PUTHTS
	MOV	BX,OFFSET OPXTHL
	CALL	PUTOPS
	CALL	SEP		; SP,EP,SP or CR, LF, HT

	MOV	BX,OFFSET OPXCHG
	CALL	PUTOPS
	CALL	PUTHTS
	MOV	BX,OFFSET OPSPBP
	CALL	PUTOPS
	JMP	PUTOPR
;
OPSPBP	DB	'SP,BP',0
OPXTHL	DB	'[BP],BX',0

;
;	CMA -> NOT AL
;
DOCMA:	CALL	PUTOPHT
	MOV	BX,OFFSET ALREG
	CALL	PUTOPS
	JMP	PUTOPR

;
;	Put 'expression' to output file.
;	'expression' is everything between
;	(xOPRND) up to the tab or spaces before
;	a ';' or CR.
;
PUTEXP:	MOV	BX,xOPRND
PUTEX1:	MOV	AL,[BX]

	CMP	AL,';'
	JNZ	LBL153
	 JMP PUTEX4
LBL153:
	CMP	AL,CR
	JNZ	LBL154
	 JMP PUTEX4
LBL154:

	CMP	AL,'!'
	JNZ	LBL155
	 JMP PUTEX3
LBL155:

PUTEX2:	INC	BX
	JMP	PUTEX1

PUTEX3:	DEC	BX
	MOV	AL,[BX]
	INC	BX
	CMP	AL,' '
	JNZ	LBL156
	 JMP PUTEX4
LBL156:
	CMP	AL,HT
	JNZ	LBL157
	 JMP PUTEX4
LBL157:
	JMP	PUTEX2

;
PUTEX4:	DEC	BX
	MOV	AL,[BX]
	CMP	AL,' '
	JNZ	LBL158
	 JMP PUTEX4
LBL158:
	CMP	AL,HT
	JNZ	LBL159
	 JMP PUTEX4
LBL159:
	INC	BX
	XCHG	BX,DX
	MOV	BX,xOPRND
PUTEX5:	MOV	AL,DH
	CMP	AL,BH
	JZ	LBL160
	 JMP PUTEX6
LBL160:
	MOV	AL,DL
	CMP	AL,BL
	JNZ	LBL161
RET
LBL161:
PUTEX6:	MOV	AL,[BX]
	CALL	PUTCHR
	INC	BX
	JMP	PUTEX5

;
;	IFC -> IF
;
DOIFC:	MOV	AL,HT
	MOV	ENDIFL,AL
	JMP	DOUPS

;
;	ICL -> *INCL
;
DOICL:	MOV	AL,HT
	MOV	ICLFLG,AL
	JMP	DOUPS

;
;	LST -> LIST
;
DOLST:	MOV	AL,HT
	MOV	LSTFLG,AL
	JMP	DOUPS

;
;	MAC -> MACRO
;
DOMAC:	MOV	AL,HT
	MOV	MACFLG,AL
DOUPS:	CALL	PUTOPHT
	MOV	AL,CR
	MOV	UPSMSG,AL
	JMP	PUTOPR

;*******************************************************
;
;	File operations
;
;*******************************************************
;
;	Set up input and output FCB's from DFCB
;
MAKFNS:	MOV	BX,OFFSET DFCB1
	MOV	DX,OFFSET INFCB
	MOV	CX,FNLEN+1
	CALL	MOVIR
	MOV	AL,[BX] ; typ specified ?
	CMP	AL,' '
	JNZ	LBL162
	 JMP MKFNS1
LBL162:
	CMP	AL,'?'
	JNZ	LBL163
	 JMP MKFNS1
LBL163:
	MOV	CX,3
	CALL	MOVIR
MKFNS1:
	MOV	BX,OFFSET DFCB1
	MOV	DX,OFFSET OUTFCB
	MOV	CX,FNLEN+1
	CALL	MOVIR

	MOV	AL,DFCB2	;
	OR	AL,AL		; allows output to
	JNZ	LBL164
	 JMP MKFNS2 ; different drive
LBL164:
	MOV	OUTFCB,AL	; than input
MKFNS2:
	MOV	AL,DFCB2+1	;
	CMP	AL,' '		;
	JNZ	LBL165
	 JMP MKFNS3 ; allows output
LBL165:
	MOV	CX,8		; file to have
	MOV	DX,OFFSET OUTFCB+1 ; different name
	MOV	BX,OFFSET DFCB2+1  ; from input file
	CALL	MOVIR		;
MKFNS3:
	MOV	AL,DFCB2+9
	CMP	AL,' '
	JNZ	LBL166
	 JMP MKFNS4
LBL166:
	MOV	CX,3
	MOV	DX,OFFSET OUTFCB+9
	MOV	BX,OFFSET DFCB2+9
	CALL	MOVIR
MKFNS4:
	MOV	DX,OFFSET PRFNM1
	CALL	PRTLIN
	MOV	BX,OFFSET INFCB
	CALL	PRFNAM
	MOV	DX,OFFSET PRFNM2
	CALL	PRTLIN
	MOV	BX,OFFSET OUTFCB
	CALL	PRFNAM
	MOV	DX,OFFSET CRLFMG
	CALL	PRTLIN
	RET

;
;	Print Filenames
;
PRFNAM:	MOV	AL,[BX] ; disk number
	OR	AL,AL
	JZ	LBL167
	 JMP PRFN1
LBL167:
	MOV	AL,xCDISK
PRFN1:	ADD	AL,'@'
	CALL	CONOUT
	MOV	AL,':'
	CALL	CONOUT
	INC	BX
	MOV	CH,8
	CALL	PRFN
	MOV	AL,'.'
	CALL	CONOUT
	MOV	CH,3
PRFN:	MOV	AL,[BX]
	INC	BX
	CMP	AL,' '
	JZ	LBL168
	 CALL CONOUT
LBL168:
	DEC	CH
	JZ	LBL169
	 JMP PRFN
LBL169:
	RET
;
PRFNM1	DB	'Source File: ',0
PRFNM2	DB	',  Destination File: ',0

;
;	Open source file with ext ASM
;
OPENIN:
	MOV	DX,OFFSET INFCB
	MOV	AH,nOPEN
	CALL	BDOS
	CMP	AL,NFF
	JNZ	LBL170
	 JMP NSFERR
LBL170:
	MOV	AL,RECLEN
	MOV	BX,OFFSET IBUFF
	ADD	BX,IBFLEN
	MOV	XIBUFF,BX
	RET
;
NSFERR:	MOV	DX,OFFSET NSFMSG ;'No Source File'
	JMP	EREXIT
;
NSFMSG	DB	'No Source File Found'
	DB	CR,LF,BEL,0
;
INFCB	DB	0,0,0,0,0,0,0,0
	DB	0,'ASM',0,0,0,0
	DB	0,0,0,0,0,0,0,0
	DB	0,0,0,0,0,0,0,0
	DB	0

;
;	Create output file with ext Z80
;
CREATO:
	MOV	DX,OFFSET OUTFCB
	MOV	AH,nOPEN
	CALL	BDOS
	CMP	AL,NFF
	JZ	LBL171
	 JMP OFEERR
LBL171:
CREAT4:	MOV	DX,OFFSET OUTFCB
	MOV	AH,nCREAT
	CALL	BDOS
	CMP	AL,NFF
	JNZ	LBL172
	 JMP NDSERR
LBL172:
	MOV	DX,OFFSET OUTFCB
	MOV	AH,nOPEN
	CALL	BDOS
	MOV	AL,RECLEN
	MOV	OBUFCT,AL
	MOV	BX,OFFSET OBUFF
	MOV	xOBUFF,BX
	RET
;
NDSERR:	MOV	DX,OFFSET NDSMSG ;'No directory space'
	JMP	EREXIT
;
NDSMSG	DB	'No Directory Space'
	DB	CR,LF,BEL,0
;
OUTFCB	DB	0,0,0,0,0,0,0,0
	DB	0,'A86',0,0,0,0
	DB	0,0,0,0,0,0,0,0
	DB	0,0,0,0,0,0,0,0
	DB	0
;
;
OFEERR:	MOV	DX,OFFSET OFEMSG ;'Output file exists'
	CALL	PRTLIN
	CALL	CHKYES
	JZ	LBL173
	 JMP ABORT
LBL173:
	MOV	DX,OFFSET OUTFCB
	MOV	AH,nDEL
	CALL	BDOS
	JMP	CREAT4
;
OFEMSG	DB	'Output File Already Exists'
	DB	' -- Delete it and Continue ? (Y/N) '
	DB	BEL,0

;*******************************************************
;
;	Put line at HL to output file until 0.
;
PUTLIN:	MOV	AL,[BX]
	AND	AL,AL
	JNZ	LBL174
RET
LBL174:
	CALL	PUTCHR
	INC	BX
	JMP	PUTLIN

;
;	Put spaces or tabs at HL to output
;	file until non-(space or tab)
;
PUTSPT:	MOV	AL,[BX]
	CMP	AL,' '
	JNZ	LBL175
	 JMP PUTSP1
LBL175:
	CMP	AL,HT
	JZ	LBL176
RET
LBL176:
PUTSP1:	CALL	PUTCHR
	INC	BX
	JMP	PUTSPT

;
;	Put statement separator to output file.
;
SEP:	MOV	BX,SEPMSG
	JMP	PUTLIN
;
SEPMSG	DW	NEWLSP
EXCLSP	DB	' ! ',0
NEWLSP	DB	CR,LF,HT,0

;
;	Put CR, LF to output file.
;
PCRLF:	MOV	AL,CR
	CALL	PUTCHR
	MOV	AL,LF

;
;	Put char in A to output file,
;	update column number.
;
PUTCHR:	PUSH	BX
	PUSH	DX
	PUSH	CX
	LAHF 
	 PUSH	AX

	MOV	TEMP,AL
	MOV	BX,xOBUFF
	CMP	AL,EOT
	JNZ	PCHR0
	MOV	AL,'!'
PCHR0:	MOV	[BX],AL
	CMP	AL,CR
	JZ	PUTCH0
	CMP	AL,LF
	JZ	PUTCH0
	CMP	AL,HT
	JNZ	PUTCH1
	MOV	AL,COLNUM
	DEC	AL
	AND	AL,NF8
	ADD	AL,N09
	JMP	PUTCH2

PUTCH0:	MOV	AL,1
	JMP	PUTCH2
;
PUTCH1:	MOV	AL,COLNUM
	INC	AL
PUTCH2:	MOV	COLNUM,AL

	INC	BX		; Inc obuff ptr
	MOV	AL,OBUFCT
	DEC	AL		; Dec obuff count
	JNZ	PTCH21
	MOV	AL,RECLEN
PTCH21:	MOV	OBUFCT,AL
	MOV	AL,BH
	CMP	AL,(SIZE OBUFF)/256
	JNZ	PUTCH4
	MOV	AL,BL
	CMP	AL,(SIZE OBUFF) MOD 256

	JNZ	PUTCH4
	MOV	DX,OFFSET OBUFF
PUTCH3:
	MOV	AH,nDMA
	PUSH	DX
	CALL	BDOS
	POP	DX

	XCHG	BX,DX

	MOV	DX,OFFSET OUTFCB
	CALL	WRTREC		;write record

	MOV	DX,OFFSET RECLEN
	ADD	BX,DX
	XCHG	BX,DX
	MOV	AL,DH
	CMP	AL,(SIZE OBUFF)/256
	JNZ	PUTCH3
	MOV	AL,DL
	CMP	AL,(SIZE OBUFF) MOD 256
	JNZ	PUTCH3

	MOV	BX,OFFSET OBUFF

PUTCH4:	MOV	xOBUFF,BX
	POP	AX 
	 SAHF
	POP	CX
	POP	DX
	POP	BX
	RET
;
TEMP	DB	0

;
;	Write record.
;
WRTREC:	MOV	AH,nWRNR
	PUSH	BX
	CALL	BDOS
	POP	BX
	AND	AL,AL
	JNZ	LBL177
RET
LBL177:
	MOV	DX,OFFSET OFWMSG ;'output file write error'
	JMP	EREXIT
;
OFWMSG	DB	'Output File Write Error'
	DB	CR,LF,BEL,0

;
;	Fill rest of obuff with EOF,
;	write record, and close file.
;
CLOSEO:	MOV	AL,EOF
	CALL	PUTCHR
	MOV	AL,OBUFCT
	CMP	AL,RECLEN
	JNZ	CLOSEO
;
CLOSE1:
	MOV	DX,OFFSET OBUFF
	MOV	BX,xOBUFF
	MOV	AL,BH
	CMP	AL,DH
	JNZ	CLOSE3
	MOV	AL,BL
	CMP	AL,DL
	JNZ	CLOSE3
CLOSE2:
	MOV	DX,OFFSET OUTFCB
	MOV	AH,nCLOSE
	JMP	BDOS
;
CLOSE3:	MOV	AH,nDMA
	PUSH	DX
	CALL	BDOS
	POP	DX
	XCHG	BX,DX
	MOV	DX,OFFSET OUTFCB
	CALL	WRTREC
	MOV	DX,RECLEN
	ADD	BX,DX
	XCHG	BX,DX
	MOV	AL,BYTE PTR xOBUFF+1
	CMP	AL,DH
	JZ	LBL178
	 JMP CLOSE3
LBL178:
	MOV	AL,BYTE PTR xOBUFF
	CMP	AL,DL
	JZ	LBL179
	 JMP CLOSE3
LBL179:
	JMP	CLOSE2

;
;	Print line at DE until 0
;	on console.
;
PRTLIN:	XCHG BX,DX 
	 MOV AL,[BX] 
	XCHG BX,DX
	AND	AL,AL
	JNZ	LBL180
RET
LBL180:
	CALL	CONOUT
	INC	DX
	JMP	PRTLIN

;
;	Console Output character in A.
;
CONOUT:	LAHF 
	 PUSH	AX
	PUSH	CX
	PUSH	DX
	PUSH	BX
	MOV	DL,AL
	MOV	DH,N00
	MOV	AH,nCOUT
	CALL	BDOS
	POP	BX
	POP	DX
	POP	CX
	POP	AX 
	 SAHF
	RET

;
;	Get char from CONSOLE and return
;	Z set if char.AND.5FH = 'Y'
;
CHKYES:	MOV	AH,nCIN
	CALL	BDOS
	LAHF 
	 PUSH	AX
	CALL	CRLF
	POP	AX 
	 SAHF
	AND	AL,5FH
	CMP	AL,'Y'
	RET

;
;	Return Z if no char available,
;	otherwise, get char in A.
;
CHKIN:	MOV	AH,nCDAV
	CALL	BDOS
	OR	AL,AL
	JNZ	LBL181
	RET
LBL181:
	MOV	AH,nCIN
	CALL	BDOS
	RET

;
;
;
CRLF:	MOV	DX,OFFSET CRLFMG
	JMP	PRTLIN
;
CRLFMG	DB	CR,LF,0

;
;	Convert upper to lower case in A.
;
LCASE:	CMP	AL,'A'
	JNC	LBL182
RET
LBL182:
	CMP	AL,'Z'+1
	JC	LBL183
RET
LBL183:
	OR	AL,20H
	RET

;
;	Convert lower case to upper case in A.
;
UCASE:	CMP	AL,'a'
	JNC	LBL184
	RET
LBL184:
	CMP	AL,'z'+1
	JC	LBL185
	RET
LBL185:
	AND	AL,5FH
	RET

;
;	Print activity dot every 100 lines.
;
PDOT:	DEC	LNCNT
	JNZ	PDOT2
	MOV	AL,'.'
	CALL	CONOUT
	MOV	LNCNT,100	; dot every 100 lines

	DEC	DOTCNT
	JNZ	PDOT1
	MOV	AL,' '
	CALL	CONOUT
	MOV	DOTCNT,10	; Space every 10 dots
PDOT1:
	DEC	NLCNT
	JNZ	PDOT2
	CALL	CRLF
	MOV	NLCNT,50	; 50 dots per line
PDOT2:	RET
;
LNCNT	DB	100		; dot every 100 lines
DOTCNT	DB	10		; Space every 10 dots
NLCNT	DB	50		; 50 dots per line

;
;	Uninitialized storage.
;
xCDISK	DB	1 DUP(?)
SSBDOS	DW	1 DUP(?)
SPBDOS	DW	1 DUP(?)
	DB	stckln DUP(?)
STACK	EQU	$
COLNUM	DB	1 DUP(?)
xOPCOD	DW	1 DUP(?)
OPCBUF	DB	opbfln DUP(?)
xOPRND	DW	1 DUP(?)
xOBUFF	DW	1 DUP(?)
OBUFCT	DB	1 DUP(?)
OBUFF	DB	obflen DUP(?)
LBUFF	DB	lbufln+3 DUP(?)
XIBUFF	DW	1 DUP(?)
IBUFF	DB	ibflen DUP(?)

code	ends

	END	HERE_FIRST

