;*******************************************************
;
;			XLT86
;
; Translates Intel 8080 assembly language source code
;  to Intel 8086 assembly language source code.
;
; 11/11/84 Frank J. Zerilli
;
VERS	EQU	105
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
;  Source file-type	 -- ASM
;  Destination file-type -- A86
;  Destination file-name -- same as the source file-name
;  Drive		 -- current drive
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
;		PUSH AX 	SAHF
;
;	  Otherwise, the translation is
;
;		LAHF		POP AX
;		XCHG AH,AL	XCHG AH,AL
;		PUSH AX 	SAHF
;		XCHG AH,AL
;
; 108H-- If zero (default) XLT86 translates
;
;		INX rp		DCX rp
;
;	 to
;
;		INC rp' 	DEC rp'
;
;	 Otherwise, the translation is
;
;		PUSHF		PUSHF
;		INC rp' 	DEC rp'
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
N09	EQU	9		; Tab every 8th col.
NF8	EQU	0F8H		; Mod 8
;
LBUFLN	EQU	80		; Line buffer length
OPBFLN	EQU	5		; Opcode buffer length
MEMSIZ	EQU	4		; Memory available in Kb
IBNUM	EQU	4*MEMSIZ
OBNUM	EQU	4*MEMSIZ
RECLEN	EQU	128		;
STCKLN	EQU	128		; Stack size
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
ABORT	EQU	0000
BDOS	EQU	0005
DFCB1	EQU	005CH
DFCB2	EQU	006CH
;
FNLEN	EQU	8
EOS	EQU	EOT		; Replacement for exclamation pt
EOF	EQU	1AH
NFF	EQU	0FFH		; Disk error return
;
;
; BDOS functions
;
NCIN	EQU	1
NCOUT	EQU	2
NCDAV	EQU	0BH
NOPEN	EQU	0FH
NCLOSE	EQU	10H
NDEL	EQU	13H
NRDNR	EQU	14H
NWRNR	EQU	15H
NCREAT	EQU	16H
NCDISK	EQU	19H
NDMA	EQU	1AH
;
;
	ORG	100H
;
;
	JMP	START
;
;
; Options flags
;
PSEFLG:	DB	0		; (103H) 0 to translate non-
				; Standard opcodes
MLTLFL:	DB	0FFH		; (104H) 0 to put input line with
				; Exc. pt. to output on one line
TCASFL:	DB	0		; (105H) 0 to preserve case
				;
LCASFL:	DB	0		; (106H) 0 for upper case if
				; TCASFL not zero
PSWFL:	DB	0		; (107H) non-zero to preserve 8080
				; Order of PSW registers on stack
INXFL:	DB	0		; (108H) non-zero to preserve flags
				; With INX and DCX translations
DADFL:	DB	0		; (109H) non-zero to preserve flags
				; With DAD translation
;
;
; Help message
;
HMSG1:	DB	CR,LF
	DB	LF
	DB	'XLT86 translates Intel 8080 assembly language source',CR,LF
	DB	'code into Intel 8086 assembly language source code.',CR,LF
	DB	LF
	DB	'It is invoked by a command of the form:',CR,LF
	DB	LF
	DB	'    XLT86 [d:]srcfile[.typ] [d:destfile.typ]',CR,LF
	DB	LF
	DB	'The brackets denote optional parameters and the ',CR,LF
	DB	'default values are:',CR,LF
	DB	LF
	DB	'    Source file-type      --  ASM',CR,LF
	DB	'    Destination file-type --  A86',CR,LF
	DB	'    Destination file-name --  same as source file-name',CR,LF
	DB	'    Drive                 --  current drive',CR,LF
	DB	CR,LF
	DB	'Press any key to continue - ',0
;
HMSG2:	DB	CR,LF
	DB	LF
	DB	'Examples:',CR,LF
	DB	LF
	DB	'XLT86 PRGM1               --translates PRGM1.ASM to PRGM1.A86',CR,LF
	DB	'XLT86 PRGM1 PRGM2         --translates PRGM1.ASM to PRGM2.A86',CR,LF
	DB	'XLT86 PRGM1.TXT PRGM2.MAC --translates PRGM1.TXT to PRGM2.MAC',CR,LF
	DB	LF
	DB	'XLT86 also has the following features:',CR,LF
	DB	LF
	DB	'Case will be preserved as well as possible -- if an opcode has',CR,LF
	DB	'a lower case character, the translated opcode will be in lower',CR,LF
	DB	'case.',CR,LF
	DB	LF
	DB	'All asterisks at the beginning of lines will be replaced with',CR,LF
	DB	'semicolons.',CR,LF
	DB	LF
	DB	'A dot is printed on the console for every 100 lines of input ',CR,LF
	DB	'processed.',CR,LF
	DB	LF
	DB	0
;.....
;
;
;=======================================================================
;
; Program begins here
;
START:	LXI	H,0
	DAD	SP
	SHLD	SPBDOS
	LXI	SP,STACK
	LDA	DFCB1+1		; Check for a file name
	CPI	' '		; Print help if no name
	JNZ	BEGIN		; No help requested
	LXI	D,SIGNON
	CALL	PRTLIN
	LXI	D,HMSG1		; Print help message
	CALL	PRTLIN
	MVI	C,NCIN		; Wait for any character
	CALL	BDOS
	LXI	D,HMSG2		; Print rest of help
	CALL	PRTLIN
	LHLD	SPBDOS		; Retrieve system stack
	SPHL			; Pointer and pop
	RET			; To PC
;.....
;
;
BEGIN:	CALL	HELLO		; Signon, open in & out files
;
NXTLIN:	CALL	GETLIN		; Get line from input file to buf
	CALL	PROCLIN		; Process line
	JMP	NXTLIN
;.....
;
;
;***********************************************************************
;
; Print signon, open input and output files.
;
HELLO:	LXI	D,SIGNON
	CALL	PRTLIN
;
HELLO0:	MVI	A,'D'		; Translate DB & EQU (for
	STA	OPTBDB		; Uniform formatting)
	MVI	A,HT		; Default is tab after opcode
	STA	MLTSPC		; For stmts on separate lines
	MVI	A,HT		; HT after opcode
	STA	PUTHT+1
	MVI	A,41		; Tab comments to col 33
	STA	PUTND5+1
	MVI	A,0FEH		; CP instruction
	STA	EXCLAM
	XRA	A
	STA	TCASFL		; Don't convert case
	STA	LCASFL		; Lower case flag
	LXI	H,NEWLSP
	SHLD	SEPMSG
	LDA	PSEFLG		; Translate non-standard
	ORA	A		; Opcodes ?
	CNZ	NXPSD
	LXI	D,DBMSG
	CALL	PRTLIN		;
	CALL	CHKYES		; Xlat DB & EQU ?
	CPI	ESC
	JZ	HELLO0
	CPI	CTRLC
	JZ	ABORT
	CPI	'Y'
	CNZ	NXDBEQ
	LDA	MLTLFL		; Force space after opcode
	ORA	A		; If MLTLFL not set
	JZ	HELLO2
	LXI	D,SPCMSG
	CALL	PRTLIN		; Use space after
	CALL	CHKYES		; Opcode ?
	CPI	ESC
	JZ	HELLO0
	CPI	CTRLC
	JZ	ABORT
	CPI	'Y'
;
HELLO2:	CZ	SETSPC
	LXI	D,COLMSG
	CALL	PRTLIN		;
	CALL	CHKYES		; Start comment
	CPI	ESC
	JZ	HELLO0
	CPI	CTRLC
	JZ	ABORT
	CPI	'Y'
	CZ	SETCOL		; In column 25 ?
	LXI	D,EXCMSG
	CALL	PRTLIN		; Ignore exclamation point
	CALL	CHKYES		; Separator ?
	CPI	ESC
	JZ	HELLO0
	CPI	CTRLC
	JZ	ABORT
	CPI	'Y'
	CZ	SETNEX
	LXI	D,MLTMSG
	CALL	PRTLIN		; Multiple statements
	CALL	CHKYES		; On one line ?
	CPI	ESC
	JZ	HELLO0
	CPI	CTRLC
	JZ	ABORT
	CPI	'Y'
	CZ	SETMLT
	LXI	D,TRNMSG	; Convert case ?
	CALL	PRTLIN
	CALL	CHKYES
	CPI	ESC
	JZ	HELLO0
	CPI	CTRLC
	JZ	ABORT
	CPI	'L'
	CZ	SETLC
	CPI	'U'
	CZ	SETUC
	MVI	A,N01
	STA	COLNUM
	MVI	C,NCDISK
	CALL	BDOS
	INR	A
	STA	XCDISK
	CALL	MAKFNS
	CALL	OPENIN
	CALL	CREATO
	RET
;.....
;
;
SIGNON:	DB	'8080-to-8086 Translator version '
	DB	VERS/100+'0','.',(VERS MOD 100)/10+'0'
	DB	(VERS MOD 10)+'0'
	DB	CR,LF,0
;.....
;
;
; Don't translate non-standard opcodes
;
NXPSD:	XRA	A
	STA	OPTTDL
	STA	OPTENT
	STA	OPTIFC
	RET
;.....
;
;
DBMSG:	DB	'Translate DB & EQU ? '
	DB	'[Y/ret=N/esc/^C] ',0
;
NXDBEQ:	XRA	A
	STA	OPTBDB
	RET
;.....
;
;
SPCMSG:	DB	'Use space (default TAB) after opcode ? '
	DB	'[Y/ret=N/esc/^C] ',0
;
SETSPC:	LXI	H,PUTHT+1
	MVI	A,' '
	MOV	M,A
	STA	MLTSPC
	RET
;.....
;
;
COLMSG:	DB	'Start comment in column 25 (default 33) ? '
	DB	'[Y/ret=N/esc/^C] ',0
;
SETCOL:	LXI	H,PUTND5+1
	MVI	A,33
	MOV	M,A
	RET
;.....
;
;
EXCMSG:	DB	'Ignore ! statement separator ? '
	DB	'[Y/ret=N/esc/^C] ',0
;
SETNEX:	MVI	A,0C9H		; RET instruction
	STA	EXCLAM
	RET
;.....
;
;
MLTMSG:	DB	'Put opcodes converted to multiple'
	DB	' statements on one line ? '
	DB	'[Y/ret=N/esc/^C] ',0
;
SETMLT:	LXI	H,EXCLSP
	SHLD	SEPMSG
	MVI	A,' '
	STA	MLTSPC
	RET
;.....
;
;
TRNMSG:	DB	'Translate instructions to Upper/Lower'
	DB	' or preserve case ? [U/L/ret/esc/^C] ',0
;
SETLC:	STA	TCASFL
	STA	LCASFL
	RET
;.....
;
;
SETUC:	STA	TCASFL
	RET
.....:
;
;
;***********************************************************************
;
; Gets line from input file to line buffer until CR.  Filters out con-
; trol characters except for horizontal tab.  Truncates lines after
; LBUFLN characters.  Terminates line with CR,LF,0.
;
GETLIN:	CALL	PDOT		; Print activity dot
	CALL	CHKIN
	CPI	CTRLC
	JZ	JABORT
	XRA	A
	STA	QUOTFL		; Not in quote
	STA	CMNTFL		; Not in comment
	LXI	H,LBUFF		; Line buffer
	MVI	B,LBUFLN	; Max # of char
;
GETLN1:	XCHG
	LHLD	XIBUFF
	XCHG
	MOV	A,D
	CPI	(IBUFF+IBFLEN)/256
	JNZ	GETLN4
	MOV	A,E
	CPI	(IBUFF+IBFLEN) MOD 256
	JNZ	GETLN4
	PUSH	B
	PUSH	H
	LXI	D,IBUFF
;
GETLN2:	MVI	C,NDMA
	PUSH	D
	CALL	BDOS
	POP	D
	XCHG
	LXI	D,INFCB
	MVI	C,NRDNR
	PUSH	H
	CALL	BDOS
	POP	H
	DCR	A
	JNZ	GETLN3
	MVI	A,EOF
	MOV	M,A
;
GETLN3:	LXI	D,RECLEN
	DAD	D
	XCHG
	MOV	A,D
	CPI	(IBUFF+IBFLEN)/256
	JNZ	GETLN2
	MOV	A,E
	CPI	(IBUFF+IBFLEN) MOD 256
	JNZ	GETLN2
	POP	H
	POP	B
	LXI	D,IBUFF
;
GETLN4:	LDAX	D
	INX	D
	XCHG
	SHLD	XIBUFF
	XCHG
	MOV	M,A
	CPI	QUOTE		; Set or reset
	JNZ	GTLN41		; QUOTFL
	LDA	QUOTFL
	CMA
	STA	QUOTFL
;
GTLN41:	MOV	A,M		; Translate exclam.  pt.
	CALL	EXCLAM		; Which is not in quote
	MOV	M,A		; To EOS
	LDA	TCASFL		; Translate to upper
	ORA	A		; Or lower case ?
	JZ	GTLN46		; NO
	LDA	QUOTFL		; If in quote, do
	ORA	A		; Nothing
	JNZ	GTLN43
	MOV	A,M		; Otherwise, ';' sets
	CPI	';'		; CMNTFL and EOS resets
	JZ	GTLN42		; It
	CPI	EOS
	JNZ	GTLN43
	XRA	A
;
GTLN42:	STA	CMNTFL
;
GTLN43:	LDA	QUOTFL		; If in quote,
	ORA	A		; Do nothing
	JNZ	GTLN46
	LDA	CMNTFL		; If in comment,
	ORA	A		; Do nothing
	JNZ	GTLN46
	LDA	LCASFL		; Otherwise,
	ORA	A		; If LCASFL set
	MOV	A,M
	JZ	GTLN44
	CALL	LCASE		; Trns to lwr case
	JMP	GTLN45
;
GTLN44:	CALL	UCASE		; Else trns to upr case
;
GTLN45:	MOV	M,A
;
GTLN46:	MOV	A,M
	CPI	CR
	JZ	GETLN6
	CPI	HT		; Filters out all ctrl
	JZ	GETLN5		; Chars except tab
	CPI	EOF
	JZ	GETLN7
	CPI	EOS
	JZ	GETLN5
	CPI	' '
	JC	GETLN1
;
GETLN5:	DCR	B
	INX	H
	JNZ	GETLN1
	INR	B
	DCX	H
	JMP	GETLN1
;
GETLN6:	INX	H
	MVI	M,LF
	INX	H
	MVI	M,N00
	XCHG
	SHLD	XIBUFF
	XCHG
	RET
;.....
;
;
; Change exclamation point to EOS in A
;
EXCLAM:	CPI	'!'
	RNZ
	LDA	QUOTFL
	ORA	A
	MVI	A,'!'
	RNZ
	MVI	A,EOS
	RET
;.....
;
;
QUOTFL:	DB	0
CMNTFL:	DB	0
;
;
; Exit
;
GETLN7:	CALL	CLOSEO
	LXI	D,UPSMSG
	CALL	PRTLIN
	LXI	D,ENDIFL
	CALL	PRTLIN
	LXI	D,ICLFLG
	CALL	PRTLIN
	LXI	D,LSTFLG
	CALL	PRTLIN
	LXI	D,MACFLG
	CALL	PRTLIN
	LXI	D,EOJMSG
;
;
; Print message at DE and abort
;
EREXIT:	PUSH	D
	LXI	D,CRLFMG
	CALL	PRTLIN
	POP	D
	CALL	PRTLIN
	JMP	ABORT
;.....
;
;
JABORT:	LXI	D,JABTMG
	CALL	PRTLIN
	JMP	GETLN7
;.....
;
;
JABTMG:	DB	CR,LF,'*** Job Cancelled ***',CR,LF,0
;
;
UPSMSG:	DB	0,LF,'The following operands'
	DB	' have been used in your '
	DB	'source and have not'
	DB	CR,LF
	DB	'been fully translated.  You must '
	DB	'complete the translation using an editor.'
	DB	CR,LF,HT
	DB	'original:',HT,HT
	DB	'must be translated to:'
	DB	CR,LF,0
;
ENDIFL:	DB	0,'IF or IFC',HT,HT,'%IF(exp)THEN(txt1)',CR,LF
	DB	HT,'ELSE',HT,HT,HT,'ELSE(txt2)',CR,LF
	DB	HT,'ENDIF or #ENDIF',HT,HT,'FI'
	DB	CR,LF,0
;
ICLFLG:	DB	0,'ICL'
	DB	CR,LF,0
;
LSTFLG:	DB	0,'LST or LIST'
	DB	CR,LF,0
;
MACFLG:	DB	0,'MACRO or MAC',HT,HT,'%DEFINE(mname[(plist)])'
	DB	CR,LF
	DB	HT,'#macro-call',HT,HT,'%macro-call'
	DB	CR,LF,0
;
EOJMSG:	DB	'*** End of Job ***',CR,LF,0
;.....
;
;
;***********************************************************************
;
; Process line
;
PROCLIN:LXI	H,LBUFF
;
PROCLN0:CALL	FNDOPC
	JNZ	PRCLN00
	CALL	PTCOLN		; Put out pending colon
	JMP	PUTND6
;...
;
;
PRCLN00:LDA	COLNFL		; Is there a colon
	ORA	A		; To print ?
	JZ	PRCLN01		; No, put white space
	LXI	H,OPTPSD	; See if opcode
	LXI	B,OPBFLN	; Is DB, DW, DS, or EQU
	CALL	SCANOP
	JZ	PRCLN01		; If so, don't put colon
	MVI	A,':'
	CALL	PUTCHR
;
PRCLN01:LHLD	XWHITE
	CALL	PUTSPT
;
PROCLN1:LXI	H,OPTIMM	; Imm or one byte
	LXI	B,2*OPBFLN
	CALL	SCANOP
	JZ	DOIMM
	LXI	H,OPTONE	; One byte opcodes
	LXI	B,2*OPBFLN
	CALL	SCANOP
	JZ	DO$ONE
	LXI	H,OPTREG	; Register
	LXI	B,2*OPBFLN
	CALL	SCANOP
	JZ	DOREG
	LXI	H,OPTBDB	; Db and equ
	LXI	B,2*OPBFLN
	CALL	SCANOP
	JZ	DOSIMP
	LXI	H,OPTSMP	; Simple
	LXI	B,2*OPBFLN
	CALL	SCANOP
	JZ	DOSIMP
	LXI	H,OPTROT	; Rotates
	LXI	B,2*OPBFLN
	CALL	SCANOP
	JZ	DOROT
	LXI	H,OPTDCR	; Dcr, inr
	LXI	B,2*OPBFLN
	CALL	SCANOP
	JZ	DODCR
	LXI	H,OPTDCX	; 16 bit dcx, inx
	LXI	B,2*OPBFLN
	CALL	SCANOP
	JZ	DODCX
	LXI	H,OPTTDL	; Tdl
	LXI	B,OPBFLN
	CALL	SCANOP
	CZ	DOTDL
	LXI	H,OPTRCC	; Ret cond
	LXI	B,2*OPBFLN
	CALL	SCANOP
	JZ	DORET
	LXI	H,OPTCCC	; Call cond
	LXI	B,2*OPBFLN
	CALL	SCANOP
	JZ	DOCALL
	LXI	H,OPTJCC	; Jump cond
	LXI	B,2*OPBFLN
	CALL	SCANOP
	JZ	DOJMP
	LXI	H,OPTMSC	; Index & misc
	LXI	B,2*OPBFLN+2
	CALL	SCANOP
	JZ	EXEC
;
PUTCOD:	LHLD	XOPCOD		; This fix prevents macro
	JMP	PUTEND		; Names from being split
;
PUTOPR:	LHLD	XOPRND
;
PUTEND:	XRA	A
	STA	LCFLAG
	STA	LCDFLG
	MVI	C,N00		; Putout w/o
;
PUTND1:	MOV	A,M		; Change
	CPI	' '
	JZ	PUTND3
	CPI	HT
	JZ	PUTND3
	CPI	CR
	JZ	PUTLNC
	CPI	';'
	JZ	PUTND4
	CPI	EOS		; Process exclamation pt.
	JZ	PTND21		; Statement separator
	CPI	QUOTE
	JNZ	PUTND2
	DCR	C
	JZ	PUTND2
	MVI	C,N01
;
PUTND2:	CALL	PUTCHR
	INX	H
	JMP	PUTND1
;
PTND21:	CALL	SKSPHT
	INX	H		; Inc past excl. pt.
;
PTND22:	MOV	A,M
	CPI	';'
	JZ	PUTND5
	LDA	MLTLFL		; Put out as separate
	ORA	A		; Lines
	JZ	PTND24		; NO
	MOV	A,M
	CPI	' '		; Change space to HT
	JNZ	PTND23
	MVI	M,HT
;
PTND23:	CALL	PCRLF
	JMP	PROCLN0
;
PTND24:	LDA	TEMP		; Was last character put
	CPI	' '		; Out a space ?
	JZ	PTND25
	CPI	HT		; Or a TAB ?
	JZ	PTND25
	MVI	A,' '		; NO, put out a space
	CALL	PUTCHR
PTND25:	MVI	A,'!'
	CALL	PUTCHR
	JMP	PROCLN0
;.....
;
;
PUTND3:	PUSH	H		; Space or Tab come here
	CALL	SKSPHT
	CPI	CR		; This fix filters out
	JZ	PUTLNB		; Trailing spaces or tabs
	POP	H
	CPI	EOS		; Fix to process excl. pt.
	JZ	PTND21
	CPI	';'
	MOV	A,M		; Prevent blank being replaced
	JZ	PUTND4
	CALL	PUTSPT
	JMP	PUTND1
;
PUTND4:	DCR	C		; ';' come here
	INR	C
	JNZ	PUTND2
	CALL	SKSPHT
;
;
; Tab comments to proper column
;
PUTND5:	MVI	B,41
;
PTND51:	LDA	COLNUM
	CMP	B		; Colnum>=41?
	JNC	PTND54
	DCR	A		; No, insert
	ANI	NF8		; Tabs to
	ADI	N09		; Start output
	CMP	B		; At col. 33
	JZ	PTND54
	JC	PTND52
	MVI	A,' '
	JMP	PTND53
;
PTND52:	MVI	A,HT
;
PTND53:	CALL	PUTCHR
	JMP	PTND51
;
PTND54:	LDA	TEMP		; Insure
	CPI	' '		; Space
	JZ	PUTND6		; Before
	CPI	HT		; Semi-colon
	JZ	PUTND6
	MVI	A,' '
	CALL	PUTCHR
;
PUTND6:	MOV	A,M
	INX	H
	CPI	EOS
	JZ	PTND22
	ORA	A
	RZ
	CALL	PUTCHR
	JMP	PUTND6
;.....
;
;
; Put line at HL to output file until 0 and reset colnum to 1.
;
PUTLNB:	XTHL			; Filter trailing
	POP	H		; Blanks or tabs
;
PUTLNC:	JMP	PUTLIN
;.....
;
;
;***********************************************************************
;
; Process labels, find potential opcode.
;
FNDOPC:	XRA	A
	STA	COLNFL		; Reset colon flag
	MOV	A,M
	CPI	' '
	JZ	FNDOP3
	CPI	HT
	JZ	FNDOP3
	CPI	CR		; Pass blank
	RZ			; Lines
	CPI	EOS		; Excl. pt. separator
	RZ
	CPI	';'
	RZ
	CPI	'*'		; Asterisk in first column
	JNZ	FNDOP1		; Is a comment line
	MVI	M,';'
	RET
;.....
;
;
; Only comes here if this is a label
;
FNDOP1:	MVI	C,N00
	MVI	A,':'		; Set colon flag
	STA	COLNFL		; To insure colon after label
;
FNDOP2:	MOV	A,M
	CPI	':'
	JZ	FNDOP4
	CPI	HT
	JZ	FNDOP6
	CPI	' '
	JZ	FNDOP6
	CPI	CR
	RZ
	CPI	EOS
	RZ
	CPI	';'
	JZ	FNDOP7
	CALL	PUTCHR
	INX	H
	INR	C
	JMP	FNDOP2
;.....
;
;
; Comes here only if space or tab at beginning of line.

FNDOP3:	PUSH	H
	CALL	SKSPHT		; Find first non-sp or tab
	CPI	CR
	JZ	FNDOP9
	CPI	EOS
	JZ	FNDOP9
	POP	H
	CALL	PUTSPT		; Print until non-sp or ht
	PUSH	H
	CALL	FINDLM		; Find ,:+-/*); CR HT or SP at HL
	CPI	':'
	POP	H
	JZ	FNDOP1
	JMP	FNDOP7
;
;
; Colon terminating label comes here
;
FNDOP4:	INX	H
	MOV	A,M
	CPI	':'
	JNZ	FNDOP5
	CALL	PUTCHR
	INX	H
;
FNDOP5:	MVI	A,':'
	STA	COLNFL		; Set colon flag
;
;
; HT or SP comes here
;
; See if there is an opcode field
;
FNDOP6:
FNDOP7:	PUSH	H
	CALL	SKSPHT
	MOV	A,M
	CPI	CR
	JZ	FNDOP9		; Filter trailing SP or TAB
	CPI	EOS
	JZ	FNDOP9		; Excl. pt. separator
	CPI	';'
	JNZ	FNDOP8
	CALL	PTCOLN		; Put colon out if flag set
	XTHL
	POP	H
	POP	B		; Clear return
	JMP	PUTND5		; Tab to proper column
;.....
;
;
; Have located opcode field
;
FNDOP8:	POP	H
	SHLD	XWHITE
	CALL	SKSPHT
;
;
; Move potential opcode to OPCBUF
;
MOVOPC:	SHLD	XOPCOD
	MVI	B,OPBFLN
	LXI	D,OPCBUF
	CALL	MOVBDH		; Move up to B char from HL to
	CALL	SKSPHT		; DE until ,:+-/*); CR HT SP
	SHLD	XOPRND
	SUB	A
	INR	A
	RET
;.....
;
;
; Come here on CR to filter trailing SP or TAB
;
FNDOP9:	CALL	PTCOLN		; Put out colon if flag set
	XTHL
	POP	H
	XRA	A
	RET
;.....
;
;
; Put colon to output file if colon flag is set.
;
PTCOLN:	LDA	COLNFL
	ORA	A
	RZ
	CALL	PUTCHR
	XRA	A
	STA	COLNFL
	RET
;.....
;
;
COLNFL:	DB	0
XWHITE:	DW	0		; Pointer to white space after
;				; Label and before opcode
;
;***********************************************************************
;
; Opcode tables
;
OPTIMM:	DB	'ACI  ADC  '
	DB	'ADI  ADD  '
	DB	'ANI  AND  '
	DB	'CPI  CMP  '
	DB	'ORI  OR   '
	DB	'SBI  SBB  '
	DB	'SUI  SUB  '
	DB	'XRI  XOR  '
	DB	0
;
OPTONE:	DB	'RET  RET  '
	DB	'CMC  CMC  '
	DB	'HLT  HLT  '
	DB	'STC  STC  '
	DB	'DAA  DAA  '
	DB	'DI   CLI  '
	DB	'EI   STI  '
	DB	'NOP  NOP  '
	DB	0
;
OPTREG:	DB	'ADC  ADC  '
	DB	'ADD  ADD  '
	DB	'ANA  AND  '
	DB	'CMP  CMP  '
	DB	'ORA  OR   '
	DB	'SBB  SBB  '
	DB	'SUB  SUB  '
	DB	'XRA  XOR  '
	DB	0
;
OPTPSD:	DB	'DB   '
	DB	'EQU  '
	DB	'DW   '
	DB	'DS   '
	DB	0
;
OPTBDB:	DB	'DB   DB   '
	DB	'EQU  EQU  '
	DB	0
;
OPTSMP:	DB	'JMP  JMP  '
	DB	'CALL CALL '
	DB	'DS   RS   '
	DB	'DW   DW   '
	DB	'SET  EQU  '
;
OPTENT:	DB	'ENT  ENTRY'
	DB	'NAM  NAME '
	DB	'RAM  DATA '
	DB	'ROG  REL  '
	DB	0
;
OPTDCR:	DB	'DCR  DEC  '
	DB	'INR  INC  '
	DB	0
;
OPTROT:	DB	'RAL  RCL  '
	DB	'RAR  RCR  '
	DB	'RLC  ROL  '
	DB	'RRC  ROR  '
	DB	0
;
OPTDCX:	DB	'DCX  DEC  '
	DB	'INX  INC  '
	DB	0
;
OPTTDL:	DB	'REQ  '
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
;
OPTRCC:	DB	'RC   JNC  '
	DB	'RNC  JC   '
	DB	'RZ   JNZ  '
	DB	'RNZ  JZ   '
	DB	'RP   JS   '
	DB	'RM   JNS  '
	DB	'RPE  JPO  '
	DB	'RPO  JPE  '
	DB	0
;
OPTCCC:	DB	'CC   JNC  '
	DB	'CNC  JC   '
	DB	'CZ   JNZ  '
	DB	'CNZ  JZ   '
	DB	'CP   JS   '
	DB	'CM   JNS  '
	DB	'CPE  JPO  '
	DB	'CPO  JPE  '
	DB	0
;
OPTJCC:	DB	'JC   JNC  '
	DB	'JNC  JC   '
	DB	'JZ   JNZ  '
	DB	'JNZ  JZ   '
	DB	'JP   JS   '
	DB	'JM   JNS  '
	DB	'JPE  JPO  '
	DB	'JPO  JPE  '
	DB	0
;
OPTMSC:	DB	'LXI  MOV  '
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
;
OPTIFC:	DB	'IFC  IF   '
	DW	DOIFC
	DB	'ICL  *INCL'
	DW	DOICL
	DB	'LST  LIST '
	DW	DOLST
	DB	'MAC  MACRO'
	DW	DOMAC
	DB	0
;.....
;
;
;***********************************************************************
;
; Scan table at HL for match to OPBFLN character string at OPCBUF.  Ret
; Z and HL-> entry if match.
;
SCANOP:	MOV	A,M
	ANA	A
	JZ	SCNOP1
	PUSH	B
	MVI	B,OPBFLN
	LXI	D,OPCBUF
	CALL	CBDEHL		; Comp B bytes (DE)-(HL)
	POP	B
	RZ
	DAD	B
	JMP	SCANOP
;
SCNOP1:	INR	A
	RET
;.....
;
;
; Gets routine address from HL+2*OPBFLN and jumps to routine.
;
EXEC:	PUSH	H
	LXI	B,2*OPBFLN
	DAD	B
	MOV	C,M
	INX	H
	MOV	B,M
	POP	H
	PUSH	B		; Address on stack
	RET			; Go to it
;.....
;
;
; Put up to OPBFLN character at HL+OPBFLN to output file.  Stop at space
; and put tab to output file.
;
PUTOPHT:CALL	PUTOPC
;
PUTHT:	MVI	A,HT
	JMP	PUTCHR
;.....
;
;
; Put space or tab (contents of MLTSPC) to output file as separator
; after opcode in multi-statement output line.
;
PUTHTS:	LDA	MLTSPC
	JMP	PUTCHR
;.....
;
;
MLTSPC:	DB	HT
;
;
PUTOPC:	LXI	B,OPBFLN
	DAD	B		; HL -> new opcode
	MOV	B,C
;
PUTOP1:	MOV	A,M
	CPI	' '
	RZ
	CPI	HT
	RZ
	LDA	LCFLAG
	ORA	A
	MOV	A,M
	JZ	PUTOP2
	ORI	20H
;
PUTOP2:	CALL	PUTCHR
	INX	H
	DCR	B
	JNZ	PUTOP1
	RET
;.....
;
;
; Put string at HL to output file until 0.  If (LCFLAG) set, convert to
; lower case.
;
PUTOPS:	MOV	A,M
	ORA	A
	RZ
	LDA	LCFLAG
	ORA	A
	MOV	A,M
	JZ	PUTOS0
	CALL	LCASE
;
PUTOS0:	CALL	PUTCHR
	INX	H
	JMP	PUTOPS
;.....
;
;
; Put string at HL to output file until 0.  If (LCDFLG) set, convert to
; lower case.
;
PUTRND:	MOV	A,M
	ORA	A
	RZ
	LDA	LCDFLG
	ORA	A
	MOV	A,M
	JZ	PUTRN0
	CALL	LCASE
;
PUTRN0:	CALL	PUTCHR
	INX	H
	JMP	PUTRND
;.....
;
;
LCDFLG:	DB	0
;
;
; Find first  ,:+-/*); CR HT or SP at HL, return A = (HL).
;
FINDLM:	PUSH	B
	CALL	CHKDLM
	POP	B
	RZ
	INX	H
	JMP	FINDLM
;.....
;
;
; Fill B locations at DE with spaces.  Move up to B character from HL to
; DE until  ,:+-/*); CR HT or SP encountered.  Return Z and HL->special
; character if found.  (Search B+1 loc for special char.)
;
MOVBDH:	MOV	C,B
	MVI	B,N00
	PUSH	B
	PUSH	D
	PUSH	H		; Fill BC locations
	CALL	FILLBD		; At DE with spaces
	POP	H
	POP	D
	POP	B
;
MOVBD1:	PUSH	B		; Ret Z, A=(HL)
	CALL	CHKDLM		; If (HL) is
	POP	B		; ,:+-/*); CR HT or SP
	RZ
	MOV	A,M
	STAX	D
	INX	D
	INX	H
	DCX	B
	MOV	A,B
	ORA	C
	JZ	CHKDLM
	JMP	MOVBD1
;.....
;
;
; Skip spaces and tabs.  Return HL -> non-space or non-tab
;
SKSPHT:	MOV	A,M
	CPI	' '
	JZ	SKSPT1
	CPI	HT
	RNZ
;
SKSPT1:	INX	H
	JMP	SKSPHT

;.....
;
;
; Ret Z, A=(HL) if HL is  ,:+-/*); CR HT SP or EOS
;
CHKDLM:	MOV	A,M
	CPI	HT
	RZ
	CPI	' '
	RZ
	CPI	','
	RZ
	CPI	';'
	RZ
	CPI	CR
	RZ
	CPI	':'
	RZ
	CPI	'+'
	RZ
	CPI	'-'
	RZ
	CPI	'/'
	RZ
	CPI	'*'
	RZ
	CPI	')'
	RZ
	CPI	EOS
	RET
;.....
;
;
; Compares B characters at DE with characters at HL.  Ret Z if match.
; Preserve HL, DE, BC
;
CBDEHL:	PUSH	H
	PUSH	D
	PUSH	B
;
CBDH1:	LDAX	D
	CPI	'a'
	JC	CBDH2
	STA	LCFLAG
	ANI	05FH
;
CBDH2:	CMP	M
	JNZ	CBDH3
	INX	H
	INX	D
	DCR	B
	JNZ	CBDH1
;
CBDH3:	POP	B
	POP	D
	POP	H
	RET
;.....
;
;
LCFLAG:	DB	0
;
;
; Fill BC locations starting at DE with spaces.  Returns A = space,
; DE -> next free location, HL = DE - 1, BC = 0.
;
FILLBD:	MVI	A,' '
	STAX	D
	MOV	H,D
	MOV	L,E
	INX	D
	DCX	B
	CALL	MOVIR
	RET
;.....
;
;
; Subtract DE from HL, result in HL.
;
SBCHLDE:MOV	A,L
	SUB	E
	MOV	L,A
	MOV	A,H
	SBB	D
	MOV	H,A
	RET
;.....
;
;
; (DE)=(HL), INC HL, INC DE, DEC BC - repeat until BC = 0.
;
MOVIR:	MOV	A,M
	STAX	D
	INX	H
	INX	D
	DCX	B
	MOV	A,B
	ORA	C
	JNZ	MOVIR
	RET
;.....
;
;
;***********************************************************************
;
;	Translation Routines
;
;***********************************************************************
;
; Immediate i.e., CPI n -> CMP AL,n
;
DOIMM:	CALL	PUTOPHT
	LXI	H,OPALC
	CALL	PUTOPS
	JMP	PUTOPR
;.....
;
;
OPALC:	DB	'AL,',0
;
;
; One byte (implied) i.e., DI -> CLI
;
DO$ONE:	CALL	PUTOPC
	JMP	PUTOPR
;.....
;
;
; Simple translation i.e., DS n -> RS n
;
DOSIMP:	CALL	PUTOPHT
	JMP	PUTOPR
;.....
;
;
; Register instructions i.e., XRA r -> XOR AL,r'
;
DOREG:	CALL	PUTOPHT		; Put out opcode+tab
	LXI	H,OPALC		; Put out 'AL,'
	CALL	PUTOPS
	LHLD	XOPRND
	CALL	TRNRG		; Translate register
	JNZ	PUTEND
	XCHG			; Get HL -> r'
	CALL	PUTRND
	XCHG
	JMP	PUTEND
;.....
;
;
; MOV r,s -> MOV r',s'
;
DOMOV:	CALL	PUTOPHT
	LHLD	XOPRND
	CALL	TRNRG
	JNZ	PUTOPR
	XCHG
	CALL	PUTRND
	XCHG
	XRA	A
	STA	LCDFLG		; Clear lower case flag
	MOV	A,M		; Get comma
	INX	H		; Inc past ','
	CALL	PUTCHR
	CALL	TRNRG
	JNZ	PUTEND
	XCHG
	CALL	PUTRND
	XCHG
	JMP	PUTEND
;.....
;
;
; Decrement and increment byte register
;
; DCR r -> DEC r'
;
; MVI r,n -> MOV r',n
;
DODCR:
DOMVI:	CALL	PUTOPHT
	LHLD	XOPRND
	CALL	TRNRG
	JNZ	PUTEND
	LDAX	D
	CPI	'['
	JNZ	DOMVI1
	PUSH	H
	LXI	H,OPBYTP
	CALL	PUTLIN
	POP	H
;
DOMVI1:	XCHG
	CALL	PUTRND
	XCHG
	JMP	PUTEND
;.....
;
;
OPBYTP:	DB	'BYTE PTR ',0
;
;
; Translate 8080 byte registers to 8086 byte registers.  Enter with
; HL -> to 8080 register.  If match, return Z set, HL -> next, DE ->
; translation, otherwise, return NZ, HL, DE unchanged.
;
TRNRG:	MOV	A,M
	CPI	'a'
	JC	TRNRG2
	STA	LCDFLG
;
TRNRG2:	ANI	5FH
	PUSH	H
	LXI	H,RTBL
	MVI	B,RTBLE-RTBL
;
TRNRG3:	CMP	M
	JZ	TRNRG4
	INX	H
	DCR	B
	JNZ	TRNRG3
	POP	H		; HL -> r
	MVI	A,0FFH		; Return NZ
	ORA	A		; If no match
	RET
;...
;
;
TRNRG4:	LXI	D,RTBL
	CALL	SBCHLDE
	DAD	H
	LXI	D,RPTBL
	DAD	D
	MOV	E,M
	INX	H
	MOV	D,M
	POP	H
	INX	H		; Point to next
	XRA	A		; Return Z
	RET
;.....
;
;
RTBL:	DB	'ABCDEHLM'
;
RTBLE:
RPTBL:	DW	ALREG
	DW	CHREG
	DW	CLREG
	DW	DHREG
	DW	DLREG
	DW	BHREG
	DW	BLREG
	DW	PBX
;.....
;
;
ALREG:	DB	'AL',0
CHREG:	DB	'CH',0
CLREG:	DB	'CL',0
DHREG:	DB	'DH',0
DLREG:	DB	'DL',0
BHREG:	DB	'BH',0
BLREG:	DB	'BL',0
PBX:	DB	'[BX]',0
;.....
;
;
; Rotates
;
DOROT:	CALL	PUTOPHT
	LXI	H,OPALC		; 'AL,'
	CALL	PUTOPS
	MVI	A,'1'
	CALL	PUTCHR
	JMP	PUTOPR
;.....
;
;
; DAD rp -> ADD BX,rp'
;
DODAD:	LDA	DADFL
	ORA	A
	JZ	DODAD1
	PUSH	H
	LXI	H,OPPSHF	; 'PUSHF'
	CALL	PUTOPS
	CALL	SEP		; Separator
	POP	H
	CALL	PUTOPC
	CALL	PUTHTS		; Put out space or tab
	JMP	DODAD2
;
DODAD1:	CALL	PUTOPHT
;
DODAD2:	LXI	H,OPBXC		; 'BX,'
	CALL	PUTOPS
	LHLD	XOPRND
	CALL	TRNRP		; DE -> translated rp
	JNZ	PUTOPR
	LDAX	D
	CPI	'A'
	JZ	PUTOPR
	XCHG			; HL -> translated rp
	CALL	PUTRND
	XCHG			; HL -> next
	LDA	DADFL
	ORA	A
	JZ	PUTEND
	PUSH	H
	CALL	SEP		; Separator
	LXI	H,OPPOPF	; 'POPF'
	CALL	PUTOPS
	POP	H
	JMP	PUTEND
;.....
;
;
OPBXC:	DB	'BX,',0
;
;
; DCX or INX rp -> DEC or INC rp'
;
DODCX:	LDA	INXFL		; If set, output
	ORA	A		; PUSHF to preserve
	JZ	DODCX1		; Flags
	PUSH	H
	LXI	H,OPPSHF	; 'PUSHF'
	CALL	PUTOPS
	CALL	SEP		; Separator
	POP	H
	CALL	PUTOPC
	CALL	PUTHTS		; Put out space or tab
	JMP	DODCX2
;
DODCX1:	CALL	PUTOPHT
;
DODCX2:	LHLD	XOPRND
	CALL	TRNRP		; DE -> translated rp
	JNZ	PUTOPR
	LDAX	D
	CPI	'A'
	JZ	PUTOPR
	XCHG			; HL -> translated rp
	CALL	PUTRND
	XCHG			; HL -> next
	LDA	INXFL		; If set, output
	ORA	A		; POPF to preserve
	JZ	PUTEND		; Flags
	PUSH	H
	CALL	SEP		; Separator
	LXI	H,OPPOPF	; 'POPF'
	CALL	PUTOPS
	POP	H
	JMP	PUTEND
;.....
;
;
OPPSHF:	DB	'PUSHF',0
OPPOPF:	DB	'POPF',0
;
;
; PUSH rp -> PUSH rp'
;
DOPSH:	XCHG
	LHLD	XOPRND
	MOV	A,M
	ANI	5FH
	CPI	'P'
	XCHG
	JNZ	DOPSH1
	XCHG
	CALL	TRNRP		; DE -> trans, HL -> next
	JNZ	PUTCOD
	PUSH	H
	LXI	H,OPLAHF
	CALL	PUTRND
	LDA	PSWFL		; Preserve order of
	ORA	A		; Registers on stack ?
	CNZ	XAHAL		; Yes,	XCHG AH,AL
	CALL	SEP		; SP,EP,SP or CR, LF, HT
	LXI	H,OPPUSH
	CALL	PUTOPS
	CALL	PUTHTS
	POP	H
	XCHG			; HL -> translated rp
	CALL	PUTRND
	XCHG			; HL -> next
	LDA	PSWFL
	ORA	A
	CNZ	XAHAL
	JMP	PUTEND
;.....
;
;
DOPSH1:	CALL	PUTOPHT
;
DOPSH2:	LHLD	XOPRND
	CALL	TRNRP		; DE -> translated rp
	JNZ	PUTOPR
;
DOPSH3:	XCHG			; HL -> translated rp
	CALL	PUTRND
	XCHG			; HL -> next
	JMP	PUTEND
;.....
;
;
OPLAHF:	DB	'LAHF',0
OPPUSH:	DB	'PUSH',0
;
XAHAL:	PUSH	H
	CALL	SEP
	LXI	H,OPXCHG
	CALL	PUTRND
	CALL	PUTHTS
	LXI	H,OPAHAL
	CALL	PUTRND
	POP	H
	RET
;.....
;
;
OPXCHG:	DB	'XCHG',0
OPAHAL:	DB	'AH,AL',0
;
;
; POP rp -> POP rp'
;
DOPOP:	XCHG
	LHLD	XOPRND
	MOV	A,M
	ANI	5FH
	CPI	'P'
	XCHG
	JNZ	DOPSH1
	CALL	PUTOPC
	CALL	PUTHT		; Put out space or tab
	LHLD	XOPRND
	CALL	TRNRP		; DE -> trans, HL -> next
	JNZ	PUTOPR
	XCHG
	CALL	PUTRND
	LDA	PSWFL
	ORA	A
	CNZ	XAHAL
	CALL	SEP		; SP,EP,SP or CR, LF, HT
	LXI	H,OPSAHF
	CALL	PUTRND
	XCHG
	JMP	PUTEND
;.....
;
;
OPSAHF:	DB	'SAHF',0
;
;
; LXI rp,nn -> MOV rp',OFFSET nn
;
DOLXI:	CALL	PUTOPHT
	LHLD	XOPRND
	CALL	TRNRP
	JNZ	PUTOPR
	LDAX	D
	CPI	'A'
	JZ	PUTOPR
	XCHG
	CALL	PUTRND
	LXI	H,OFFATR
	CALL	PUTLIN
	XCHG			; HL -> next
	INX	H		; Skip comma
	JMP	PUTEND
;.....
;
;
OFFATR:	DB	',OFFSET ',0
;
;
; Translate 16 bit registers.  Enter with HL -> rp.  Returns HL -> next
; character, DE -> translation, Z set if match, otherwise, HL unchanged,
; NZ.
;
TRNRP:	XRA	A
	STA	LCDFLG
	MOV	A,M
	CPI	'a'
	JC	TRNRP1
	STA	LCDFLG
;
TRNRP1:	ANI	5FH
	CPI	'B'
	JZ	TRNRPB
	CPI	'D'
	JZ	TRNRPD
	CPI	'H'
	JZ	TRNRPH
	CPI	'P'
	JZ	TRNRPP
	CPI	'S'
	JZ	TRNRPS
;
TRNRP2:	MVI	A,0
	STA	LCDFLG
	RET
;...
;
;
TRNRPB:	LXI	D,OPRCX		; 'CX'
	INX	H
	RET
;...
;
;
TRNRPD:	LXI	D,OPRDX		; 'DX'
	INX	H
	RET
;...
;
;
TRNRPH:	LXI	D,OPRBX		; 'BX'
	INX	H
	RET
;...
;
;
TRNRPP:	INX	H
	MOV	A,M
	ANI	5FH
	CPI	'S'
	JNZ	TRNRP4
	INX	H
	MOV	A,M
	ANI	5FH
	CPI	'W'
	JNZ	TRNRP3
	LXI	D,OPRAX		; 'AX'
	INX	H
	RET
;...
;
;
TRNRP3:	DCX	H
;
TRNRP4:	DCX	H
	JMP	TRNRP2
;...
;
;
TRNRPS:	INX	H
	MOV	A,M
	ANI	5FH
	CPI	'P'
	JNZ	TRNRP4
	LXI	D,OPRSP
	INX	H
	RET
;.....
;
;
OPRAX:	DB	'AX',0
OPRCX:	DB	'CX',0
OPRDX:	DB	'DX',0
OPRBX:	DB	'BX',0
OPRSP:	DB	'SP',0
;.....
;
;
;Strange opcodes
;
DOTDL:	LDA	OPCBUF+1
	LXI	H,CCZ		; 'Z '
	CPI	'E'
	JZ	DOTDL1
	LXI	H,CCNZ		; 'NZ'
	CPI	'N'
	JZ	DOTDL1
	LXI	H,CCC		; 'C '
	CPI	'L'
	JZ	DOTDL1
	LXI	H,CCNC		; 'NC'
	CPI	'G'
	JZ	DOTDL1
	LXI	H,CCZL
	CPI	'e'
	JZ	DOTDL1
	LXI	H,CCNZL
	CPI	'n'
	JZ	DOTDL1
	LXI	H,CCCL
	CPI	'l'
	JZ	DOTDL1
	LXI	H,CCNCL
;
DOTDL1:	MOV	A,M
	STA	OPCBUF+1
	INX	H
	MOV	A,M
	STA	OPCBUF+2
	RET
;.....
;
;
CCZ:	DB	'Z '
CCNZ:	DB	'NZ'
CCC:	DB	'C '
CCNC:	DB	'NC'
CCZL:	DB	'z '
CCNZL:	DB	'nz'
CCCL:	DB	'c '
CCNCL:	DB	'nc'
;.....
;
;
; Return conditional RC -> JNC $+3, RET
;
DORET:	CALL	PUTOPC
	CALL	PUTHT		; Could be PUTHTS
	LXI	H,REL3
	CALL	PUTLIN
	CALL	SEP
	LXI	H,OPRET
	CALL	PUTOPS
	JMP	PUTOPR
;.....
;
;
REL3:	DB	'$+3',0
OPRET:	DB	'RET',0
;
;
; Call conditional CC label -> JNC $+5, CALL label
;
DOCALL:	CALL	PUTOPC
	CALL	PUTHT		; Could be PUTHTS
	LXI	H,REL5
	CALL	PUTLIN
	CALL	SEP		; SP,EP,SP or CR, LF, HT
	LXI	H,OPCALL
	CALL	PUTOPS
	CALL	PUTHTS
	JMP	PUTOPR
;.....
;
;
REL5:	DB	'$+5',0
OPCALL:	DB	'CALL',0
;
;
; Jump conditional JC label -> JNC $+5, JMP label
;
DOJMP:	CALL	PUTOPC
	CALL	PUTHT		; Could be PUTHTS
	LXI	H,REL5
	CALL	PUTLIN
	CALL	SEP		; SP,EP,SP or CR, LF, HT
	LXI	H,OPJMP
	CALL	PUTOPS
	CALL	PUTHTS
	JMP	PUTOPR
;.....
;
;
OPJMP:	DB	'JMP',0
;
;
; IN n -> IN AL,n
;
DOIN:	JMP	DOIMM
;
;
; LDA addr -> MOV AL,addr
;
DOLDA:	JMP	DOIMM
;
;		   XCHG BX,rp'
; LDAX rp -> MOV AL,[BX]
;		   XCHG BX,rp'
;
DOLDAX:	LHLD	XOPRND
	CALL	TRNRP		; DE -> trans, HL -> next
	JNZ	PUTCOD
	PUSH	H
	LXI	H,OPXCHG
	CALL	PUTOPS
	CALL	PUTHTS
	LXI	H,OPBXC
	CALL	PUTOPS
	XCHG
	PUSH	H
	CALL	PUTRND
	CALL	SEP		; SP,EP,SP or CR, LF, HT
	LXI	H,OPMOV
	CALL	PUTOPS
	CALL	PUTHTS
	LXI	H,OPLDAX
	CALL	PUTOPS
	CALL	SEP		; SP,EP,SP or CR, LF, HT
	LXI	H,OPXCHG
	CALL	PUTOPS
	CALL	PUTHTS
	LXI	H,OPBXC
	CALL	PUTOPS
	POP	H		; HL -> rp'
	CALL	PUTRND
	POP	H
	JMP	PUTEND
;.....
;
;
OPMOV:	DB	'MOV',0
OPLDAX:	DB	'AL,[BX]',0
;
;
; LHLD addr -> MOV BX,addr
;
DOLHLD:	CALL	PUTOPHT
	LXI	H,OPBXC		; 'BX,'
	CALL	PUTOPS
	JMP	PUTOPR
;.....
;
;
;OUT n -> OUT n,AL
;
DOOUT:	CALL	PUTOPHT
	CALL	PUTEXP
	PUSH	H
	LXI	H,OPCAL		; ',AL'
	JMP	DOSTA1
;.....
;
;
; PCHL -> JMP BX
;
DOPCHL:	CALL	PUTOPHT
	LXI	H,OPRBX		; 'BX'
	CALL	PUTOPS
	JMP	PUTOPR
;.....
;
;
; RST -> CALL 8*
;
DORST:	CALL	PUTOPHT
	LXI	H,OPR8M		; '8*'
	CALL	PUTOPS
	JMP	PUTOPR
;.....
;
;
OPR8M:	DB	'8*',0
;
;
; SHLD addr -> MOV addr,BX
;
DOSHLD:	CALL	PUTOPHT
	CALL	PUTEXP
	PUSH	H
	LXI	H,OPCBX		; ',BX'
	JMP	DOSTA1
;.....
;
;
OPCBX:	DB	',BX',0
;
;
; SPHL -> MOV SP,BX
;
DOSPHL:	CALL	PUTOPHT
	LXI	H,OPSPBX	; 'SP,BX'
	CALL	PUTOPS
	JMP	PUTOPR
;.....
;
;
OPSPBX:	DB	'SP,BX',0
;
;
; STA addr -> MOV addr,AL
;
DOSTA:	CALL	PUTOPHT
	CALL	PUTEXP
	PUSH	H
	LXI	H,OPCAL		; ',AL'
;
DOSTA1:	CALL	PUTOPS
	POP	H
	JMP	PUTEND
;.....
;
;
OPCAL:	DB	',AL',0
;
;		   XCHG BX,rp'
; STAX rp -> MOV [BX],AL
;		   XCHG BX,rp'
;
DOSTAX:	LHLD	XOPRND
	CALL	TRNRP		; DE -> trans, HL -> next
	JNZ	PUTCOD
	PUSH	H
	LXI	H,OPXCHG
	CALL	PUTOPS
	CALL	PUTHTS
	LXI	H,OPBXC
	CALL	PUTOPS
	XCHG
	PUSH	H
	CALL	PUTRND
	CALL	SEP		; SP,EP,SP or CR, LF, HT
	LXI	H,OPMOV
	CALL	PUTOPS
	CALL	PUTHTS
	LXI	H,OPSTAX
	CALL	PUTOPS
	CALL	SEP		; SP,EP,SP or CR, LF, HT
	LXI	H,OPXCHG
	CALL	PUTOPS
	CALL	PUTHTS
	LXI	H,OPBXC
	CALL	PUTOPS
	POP	H		; HL -> rp'
	CALL	PUTRND
	POP	H
	JMP	PUTEND
;.....
;
;
OPSTAX:	DB	'[BX],AL',0
;
;
; XCHG -> XCHG BX,DX
;
DOXCHG:	CALL	PUTOPHT
	LXI	H,OPBXDX	; 'BX,DX'
	CALL	PUTOPS
	JMP	PUTOPR
;.....
;
;
OPBXDX:	DB	'BX,DX',0
;
;		XCHG SP,BP
; XTHL -> XCHG [BP],BX
;		XCHG SP,BP
;
DOXTHL:	LXI	H,OPXCHG
	CALL	PUTOPS
	CALL	PUTHTS
	LXI	H,OPSPBP	; 'SP,BP'
	CALL	PUTOPS
	CALL	SEP		; SP,EP,SP or CR, LF, HT
	LXI	H,OPXCHG
	CALL	PUTOPS
	CALL	PUTHTS
	LXI	H,OPXTHL
	CALL	PUTOPS
	CALL	SEP		; SP,EP,SP or CR, LF, HT
	LXI	H,OPXCHG
	CALL	PUTOPS
	CALL	PUTHTS
	LXI	H,OPSPBP
	CALL	PUTOPS
	JMP	PUTOPR
;.....
;
;
OPSPBP:	DB	'SP,BP',0
OPXTHL:	DB	'[BP],BX',0
;
;
; CMA -> NOT AL
;
DOCMA:	CALL	PUTOPHT
	LXI	H,ALREG
	CALL	PUTOPS
	JMP	PUTOPR
;.....
;
;
; Put 'expression' to output file.  'expression' is everything between
; (xOPRND) up to the tab or spaces before a ';' or CR.
;
PUTEXP:	LHLD	XOPRND
;
PUTEX1:	MOV	A,M
	CPI	';'
	JZ	PUTEX4
	CPI	CR
	JZ	PUTEX4
	CPI	'!'
	JZ	PUTEX3
;
PUTEX2:	INX	H
	JMP	PUTEX1
;
PUTEX3:	DCX	H
	MOV	A,M
	INX	H
	CPI	' '
	JZ	PUTEX4
	CPI	HT
	JZ	PUTEX4
	JMP	PUTEX2
;
PUTEX4:	DCX	H
	MOV	A,M
	CPI	' '
	JZ	PUTEX4
	CPI	HT
	JZ	PUTEX4
	INX	H
	XCHG
	LHLD	XOPRND
;
PUTEX5:	MOV	A,D
	CMP	H
	JNZ	PUTEX6
	MOV	A,E
	CMP	L
	RZ
;
PUTEX6:	MOV	A,M
	CALL	PUTCHR
	INX	H
	JMP	PUTEX5
;.....
;
;
; IFC -> IF
;
DOIFC:	MVI	A,HT
	STA	ENDIFL
	JMP	DOUPS
;.....
;
;
; ICL -> *INCL
;
DOICL:	MVI	A,HT
	STA	ICLFLG
	JMP	DOUPS
;.....
;
;
; LST -> LIST
;
DOLST:	MVI	A,HT
	STA	LSTFLG
	JMP	DOUPS
;.....
;
;
; MAC -> MACRO
;
DOMAC:	MVI	A,HT
	STA	MACFLG
;
DOUPS:	CALL	PUTOPHT
	MVI	A,CR
	STA	UPSMSG
	JMP	PUTOPR
;.....
;
;
;***********************************************************************
;
;	File operations
;
;***********************************************************************
;
; Set up input and output FCB's from DFCB
;
MAKFNS:	LXI	H,DFCB1
	LXI	D,INFCB
	LXI	B,FNLEN+1
	CALL	MOVIR
	MOV	A,M		; Typ specified ?
	CPI	' '
	JZ	MKFNS1
	CPI	'?'
	JZ	MKFNS1
	LXI	B,3
	CALL	MOVIR
;
MKFNS1:	LXI	H,DFCB1
	LXI	D,OUTFCB
	LXI	B,FNLEN+1
	CALL	MOVIR
	LDA	DFCB2
	ORA	A		; Allows output to
	JZ	MKFNS2		; Different drive
	STA	OUTFCB		; Than input
;
MKFNS2:	LDA	DFCB2+1
	CPI	' '
	JZ	MKFNS3		; Allows output
	LXI	B,8		; File to have
	LXI	D,OUTFCB+1	; Different name
	LXI	H,DFCB2+1	; From input file
	CALL	MOVIR
;
MKFNS3:	LDA	DFCB2+9
	CPI	' '
	JZ	MKFNS4
	LXI	B,3
	LXI	D,OUTFCB+9
	LXI	H,DFCB2+9
	CALL	MOVIR
;
MKFNS4:	LXI	D,PRFNM1
	CALL	PRTLIN
	LXI	H,INFCB
	CALL	PRFNAM
	LXI	D,PRFNM2
	CALL	PRTLIN
	LXI	H,OUTFCB
	CALL	PRFNAM
	LXI	D,CRLFMG
	CALL	PRTLIN
	RET
;.....
;
;
; Print Filenames
;
PRFNAM:	MOV	A,M		; Disk number
	ORA	A
	JNZ	PRFN1
	LDA	XCDISK
;
PRFN1:	ADI	'@'
	CALL	CONOUT
	MVI	A,':'
	CALL	CONOUT
	INX	H
	MVI	B,8
	CALL	PRFN
	MVI	A,'.'
	CALL	CONOUT
	MVI	B,3
;
PRFN:	MOV	A,M
	INX	H
	CPI	' '
	CNZ	CONOUT
	DCR	B
	JNZ	PRFN
	RET
;.....
;
;
PRFNM1:	DB	'Source File: ',0
PRFNM2:	DB	',  Destination File: ',0
;
;
; Open source file with ext ASM
;
OPENIN:	LXI	D,INFCB
	MVI	C,NOPEN
	CALL	BDOS
	CPI	NFF
	JZ	NSFERR
	MVI	A,RECLEN
	LXI	H,IBUFF+IBFLEN
	SHLD	XIBUFF
	RET
;.....
;
;
NSFERR:	LXI	D,NSFMSG	; 'No Source File'
	JMP	EREXIT
;.....
;
;
NSFMSG:	DB	'No Source File Found'
	DB	CR,LF,BEL,0
;
INFCB:	DB	0,0,0,0,0,0,0,0
	DB	0,'ASM',0,0,0,0
	DB	0,0,0,0,0,0,0,0
	DB	0,0,0,0,0,0,0,0
	DB	0
;.....
;
;
; Create output file with ext Z80
;
CREATO:	LXI	D,OUTFCB
	MVI	C,NOPEN
	CALL	BDOS
	CPI	NFF
	JNZ	OFEERR
;
CREAT4:	LXI	D,OUTFCB
	MVI	C,NCREAT
	CALL	BDOS
	CPI	NFF
	JZ	NDSERR
	LXI	D,OUTFCB
	MVI	C,NOPEN
	CALL	BDOS
	MVI	A,RECLEN
	STA	OBUFCT
	LXI	H,OBUFF
	SHLD	XOBUFF
	RET
;.....
;
;
NDSERR:	LXI	D,NDSMSG	; 'No directory space'
	JMP	EREXIT
;.....
;
;
NDSMSG:	DB	'No Directory Space'
	DB	CR,LF,BEL,0
;
OUTFCB:	DB	0,0,0,0,0,0,0,0
	DB	0,'A86',0,0,0,0
	DB	0,0,0,0,0,0,0,0
	DB	0,0,0,0,0,0,0,0
	DB	0
;.....
;
;
OFEERR:	LXI	D,OFEMSG	; 'Output file exists'
	CALL	PRTLIN
	CALL	CHKYES
	JNZ	ABORT
	LXI	D,OUTFCB
	MVI	C,NDEL
	CALL	BDOS
	JMP	CREAT4
;.....
;
;
OFEMSG:	DB	'Output File Already Exists'
	DB	' -- Delete it and Continue ? (Y/N) '
	DB	BEL,0
;.....
;
;
;***********************************************************************
;
; Put line at HL to output file until 0.
;
PUTLIN:	MOV	A,M
	ANA	A
	RZ
	CALL	PUTCHR
	INX	H
	JMP	PUTLIN
;.....
;
;
; Put spaces or tabs at HL to output file until non-(space or tab)
;
PUTSPT:	MOV	A,M
	CPI	' '
	JZ	PUTSP1
	CPI	HT
	RNZ
;
PUTSP1:	CALL	PUTCHR
	INX	H
	JMP	PUTSPT
;.....
;
;
; Put statement separator to output file.
;
SEP:	LHLD	SEPMSG
	JMP	PUTLIN
;.....
;
SEPMSG:	DW	NEWLSP
EXCLSP:	DB	' ! ',0
NEWLSP:	DB	CR,LF,HT,0
;
;
; Put CR, LF to output file.
;
PCRLF:	MVI	A,CR
	CALL	PUTCHR
	MVI	A,LF
;
;
; Put character in A to output file, update column number.
;
PUTCHR:	PUSH	H
	PUSH	D
	PUSH	B
	PUSH	PSW
	STA	TEMP
	LHLD	XOBUFF
	CPI	EOT
	JNZ	PCHR0
	MVI	A,'!'
;
PCHR0:	MOV	M,A
	CPI	CR
	JZ	PUTCH0
	CPI	LF
	JZ	PUTCH0
	CPI	HT
	JNZ	PUTCH1
	LDA	COLNUM
	DCR	A
	ANI	NF8
	ADI	N09
	JMP	PUTCH2
;
PUTCH0:	MVI	A,1
	JMP	PUTCH2
;
PUTCH1:	LDA	COLNUM
	INR	A
;
PUTCH2:	STA	COLNUM
	INX	H		; Inc obuff ptr
	LDA	OBUFCT
	DCR	A		; Dec obuff count
	JNZ	PTCH21
	MVI	A,RECLEN
;
PTCH21:	STA	OBUFCT
	MOV	A,H
	CPI	(OBUFF+OBFLEN)/256
	JNZ	PUTCH4
	MOV	A,L
	CPI	(OBUFF+OBFLEN) MOD 256
	JNZ	PUTCH4
	LXI	D,OBUFF
;
PUTCH3:	MVI	C,NDMA
	PUSH	D
	CALL	BDOS
	POP	D
	XCHG
	LXI	D,OUTFCB
	CALL	WRTREC		; Write record
	LXI	D,RECLEN
	DAD	D
	XCHG
	MOV	A,D
	CPI	(OBUFF+OBFLEN)/256
	JNZ	PUTCH3
	MOV	A,E
	CPI	(OBUFF+OBFLEN) MOD 256
	JNZ	PUTCH3
	LXI	H,OBUFF
;
PUTCH4:	SHLD	XOBUFF
	POP	PSW
	POP	B
	POP	D
	POP	H
	RET
;.....
;
;
TEMP:	DB	0
;
;
; Write record
;
WRTREC:	MVI	C,NWRNR
	PUSH	H
	CALL	BDOS
	POP	H
	ANA	A
	RZ
	LXI	D,OFWMSG	; 'output file write error'
	JMP	EREXIT
;.....
;
;
OFWMSG:	DB	'Output File Write Error'
	DB	CR,LF,BEL,0
;
;
; Fill rest of obuff with EOF,	rite record, and close file.
;
CLOSEO:	MVI	A,EOF
	CALL	PUTCHR
	LDA	OBUFCT
	CPI	RECLEN
	JNZ	CLOSEO
;
CLOSE1:	LXI	D,OBUFF
	LHLD	XOBUFF
	MOV	A,H
	CMP	D
	JNZ	CLOSE3
	MOV	A,L
	CMP	E
	JNZ	CLOSE3
;
CLOSE2:	LXI	D,OUTFCB
	MVI	C,NCLOSE
	JMP	BDOS
;.....
;
;
CLOSE3:	MVI	C,NDMA
	PUSH	D
	CALL	BDOS
	POP	D
	XCHG
	LXI	D,OUTFCB
	CALL	WRTREC
	LXI	D,RECLEN
	DAD	D
	XCHG
	LDA	XOBUFF+1
	CMP	D
	JNZ	CLOSE3
	LDA	XOBUFF
	CMP	E
	JNZ	CLOSE3
	JMP	CLOSE2
;.....
;
;
; Print line at DE until 0 on console.
;
PRTLIN:	LDAX	D
	ANA	A
	RZ
	CALL	CONOUT
	INX	D
	JMP	PRTLIN
;.....
;
;
; Console Output character in A.
;
CONOUT:	PUSH	PSW
	PUSH	B
	PUSH	D
	PUSH	H
	MOV	E,A
	MVI	D,N00
	MVI	C,NCOUT
	CALL	BDOS
	POP	H
	POP	D
	POP	B
	POP	PSW
	RET
;.....
;
;
; Get character from CONSOLE and return Z set if char.AND.5FH = 'Y'
;
CHKYES:	MVI	C,NCIN
	CALL	BDOS
	PUSH	PSW
	CALL	CRLF
	POP	PSW
	ANI	5FH
	CPI	'Y'
	RET
;.....
;
;
; Return Z if no character available, otherwise, get character in A.
;
CHKIN:	MVI	C,NCDAV
	CALL	BDOS
	ORA	A
	RZ
	MVI	C,NCIN
	CALL	BDOS
	RET
;.....
;
;
;
CRLF:	LXI	D,CRLFMG
	JMP	PRTLIN
;.....
;
;
CRLFMG:	DB	CR,LF,0
;
;
; Convert upper to lower case in A.
;
LCASE:	CPI	'A'
	RC
	CPI	'Z'+1
	RNC
	ORI	20H
	RET
;.....
;
;
; Convert lower case to upper case in A.
;
UCASE:	CPI	'a'
	RC
	CPI	'z'+1
	RNC
	ANI	5FH
	RET
;.....
;
;
; Print activity dot every 100 lines.
;
PDOT:	LDA	LNCNT
	DCR	A
	STA	LNCNT
	RNZ
	MVI	A,'.'
	CALL	CONOUT
	MVI	A,100		; Dot every 100 lines
	STA	LNCNT
	LDA	DOTCNT
	DCR	A
	STA	DOTCNT
	JNZ	PDOT1
	MVI	A,' '
	CALL	CONOUT
	MVI	A,10		; Space every 10 dots
	STA	DOTCNT
;
PDOT1:	LDA	NLCNT
	DCR	A
	STA	NLCNT
	RNZ
	CALL	CRLF
	MVI	A,50		; 50 dots per line
	STA	NLCNT
	RET
;.....
;
;
LNCNT:	DB	100		; Dot every 100 lines
DOTCNT:	DB	10		; Space every 10 dots
NLCNT:	DB	50		; 50 dots per line
;
;
; Uninitialized storage
;
XCDISK:	DS	1
SPBDOS:	DS	2
	DS	STCKLN
STACK	EQU	$
COLNUM:	DS	1
XOPCOD:	DS	2
OPCBUF:	DS	OPBFLN
XOPRND:	DS	2
XOBUFF:	DS	2
OBUFCT:	DS	1
OBUFF:	DS	OBFLEN
LBUFF:	DS	LBUFLN+3
XIBUFF:	DS	2
IBUFF:	DS	IBFLEN
;.....
;
;
	END	START
