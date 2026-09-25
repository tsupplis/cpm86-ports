; VTL2.ASM
; --------

; MITS VTL-2 -- A Very Tiny Language for the Altair 8800
; by Frank McCoy, Copyright 1976

; (Retyped by Emmanuel ROCHE.)

; Modifications and simple CP/M port by Peter Schorn, 2006
; Changes to original Altair version
; - removed all IN/OUT to Altair ports and replaced by CP/M console I/O
; - system variables ^ (uparrow), , (comma) and . (period) which deal
;   with Altair I/O specialties were removed
; - added initialization code for & (start of user program)
; - added initialization code for * (memory size)
; - moved variable storage from predefined addresses after code,
;   requiring minor change to convp
; - modified decimal print routine to deal with powers of ten table
;   where low byte comes before high byte

; Tips
; - to return to CP/M type ">=<return>"
; - to delete the user program type "&=1353<return>"
;   (initial value of the system variable &)

;--------------------------------
; ASCII characters used (ASR-33 Teletype).

ctrlC	EQU	03H		; Abort key
backs	EQU	08H		; Backspace
rubout	EQU	7FH		; DEL, sent by most terminals' Backspace key
lf	EQU	0AH		; Line Feed
cr	EQU	0DH		; Carriage Return
quote	EQU	22H		; Surrounds strings
dolar	EQU	24H		; Dollar
Lparen	EQU	28H		; Left	parenthesis
Rparen	EQU	29H		; Right parenthesis
star	EQU	2AH		; Multiplication
plus	EQU	2BH		; Addition
minus	EQU	2DH		; Subtraction
slash	EQU	2FH		; Division
zero	EQU	30H		; Number zero
colon	EQU	3AH		; Start of array statement
semicol EQU	3BH		; Semicolon after PRINT
equal	EQU	3DH		; Equal sign
greatR	EQU	3EH		; Greater sign
quest	EQU	3FH		; Question mark
atsign	EQU	40H		; Discard line

; CP/M constants
BDOS	EQU	0005H
CONSTAT	EQU	0BH		; Console status
CONIN	EQU	01H		; Console input
CONOUT	EQU	02H		; Console output
DIRCON	EQU	06H		; Direct console I/O, no echo
SETDMA	EQU	26		; Set DMA address
OPEN	EQU	15		; Open file
READ	EQU	20		; Read sequential record
FCB	EQU	05CH		; Default FCB
DMA	EQU	080H		; Default DMA buffer
bufsiz	EQU	73		; Size of input buffer
brkpnt	EQU	0000H		; Address of RST 0

;--------------------------------
;	User program
;--------------------------------

	ORG	0100H

begin:	LXI	H,prgm		; Initialize SV for next byte of
	SHLD	ampers		;	User program
; Can also restart here (0106) to keep user program
	LHLD	BDOS+1		; Get end of TPA
	LXI	D,0FC00H
	DAD	D		; Subtract space for stack
	SHLD	asterix		; Store in SV for memory size in bytes
	LHLD	BDOS+1		; Stack must be up before strng is called
	MVI	L,0
	SPHL
	XRA	A		; Initialize delimiter
	LXI	H,banner
	CALL	strng

start:	LHLD	BDOS+1		; Get end of TPA
	MVI	L,0		; Clear lower part
	SPHL			; Initialize stack pointer
	CALL	scrini		; Optional file input from default FCB
	LDA	skipok		; A loaded script suppresses this prompt once
	ORA	A
	MVI	A,0
	STA	skipok
	JNZ	loop
	XRA	A		; Initialize delimiter (after scrini, which clobbers A)
	LXI	H,okm		; Point to "OK" prompt
	CALL	strng		; Display it on terminal

; Main loop of VTL-2 interpreter.

loop:	LXI	H,0000H		; = 0
	SHLD	dollar		; Initialize char ptr
	CALL	cvtln		; Was a line number inputted?

	JNC	stmnt		; 0: List (display) user program statements

	CALL	exec		; No: Execute direct statement

	JZ	start		; Done: Return to interpreter

loop2:	CALL	find		; No: Find what remains in line?

; Start processing "=" statements.

eqstrt: JNC	start		; No: Return to interpreter

	CALL	lxhn		; Get line number, and compare with ?

	SHLD	dollar		; Save char ptr
	LHLD	number		; Get lin ptr
	INX	H
	INX	H		; 3 bytes = line number + space?
	INX	H
	CALL	exec		; Execute direct statement

	XCHG
	JZ	loop3		; Zero: ?

	LHLD	number		; Get lin ptr
	CALL	lxhn		; Get line number, and compare with ?

	JZ	loop3		; Zero: ?

	INX	H
	SHLD	exclam		; RETURN value
	JMP	loop2		; Unconditional jump to loop2

loop3:	PUSH	H		; Save line number
	LHLD	ampers		; Next byte of user program
	MOV	B,H
	MOV	C,L		; Copy it into BC
	POP	H		; Restore line number
	CALL	fnd3		; Find what?

	JMP	eqstrt		; Process "=" statement

;--------------------------------
; Get line number from user program.

lxhn:	MOV	A,M
	INX	H
	MOV	H,M
	MOV	L,A

; ... and falls through

; Compare HL with DE.

cphd:	MOV	A,L
	CMP	E
	RNZ

	MOV	A,H
	CMP	D
	RET

;--------------------------------
; Execute direct statements.

exec:	SHLD	linumb		; Save line number?
	CALL	var2

	INX	H
	MOV	A,M
	CALL	evil

	LHLD	dollar		; Get char ptr
	MOV	A,H		; 16-bits test if it is zero
	ORA	L		; (Sets Zero flag)
	RET

;--------------------------------
; List (display) user program statements.

stmnt:	SHLD	curent		; Save current line number
	MOV	L,C		; Copy BC (?) to HL
	MOV	H,B
	SHLD	dollar		; And save it in char ptr
	MOV	A,B		; 16-bits test if BC is zero
	ORA	C
	JNZ	skp2		; No: Skip what?

	LHLD	ampers		; Next byte of user program
	XCHG			; Put it in DE
	LXI	H,prgm		; Start of user program

; List (display) program on terminal.

lst2:	CALL	cphd		; Check if next = start (= no program)

	JZ	start		; Yes: Return to interpreter

	PUSH	D		; No:
	MOV	C,M
	INX	H		; Get line number from memory
	MOV	B,M		;   and put it into BC.
	PUSH	H
	CALL	prnt2		; Print 16-bit line number in decimal

	POP	H
	INX	H		; Skip the space
	CALL	pntmsg		; Print contents of line

	CALL	crlf		; End of line

	POP	D
	JMP	lst2		; Loop until end of program

;--------------------------------
; Next text?

nxtxt:	LHLD	number		; Lin ptr
	INX	H
lookag: INX	H
	MOV	A,M
	ANA	A
	JNZ	lookag		; Look again...

; "Output a char" (at the end) uses those 2 instructions...

outd:	INX	H
	RET

find:	LHLD	ampers		; Next byte of user program
	MOV	C,L
	MOV	B,H		; Copy it to BC
	LHLD	dollar		; Char ptr
	XCHG			; Copy it to DE
	LXI	H,prgm		; PC of user program
fnd2:	SHLD	number		; Set it equal to lin ptr
	MOV	A,C
	CMP	L		; Compare low byte of user program
	JNZ	nxtuu

	MOV	A,B		; Compare high byte of user program
	CMP	H
	RZ			; Zero: Return

; Not zero: ?

nxtuu:	MOV	A,M		; Get low byte of user program
	SUB	E		; Subtract low byte of char ptr
	INX	H		; Point to next byte
	MOV	A,M		; Get high byte of user program
	SBB	D		; Subtract low byte of char ptr
	DCX	H		; Point to previous byte
	CMC			; Complement Carry flag
	RC			; Return if DE > M(HL)

; Else, call ?

fnd3:	CALL	nxtxt

	JMP	fnd2

evil:	CPI	quote		; Is it a quote surrounding strings?
	JZ	quote2		; Yes: go process it

	CALL	eval		; No: Evaluate expression

	PUSH	B
	LHLD	linumb
	CALL	convp

	POP	B
	CPI	dolar		; Is it a single character?
	JNZ	andt		; No: Check if it is a "?" char

	MOV	A,C		; Yes:
	JMP	outch		;   Output it to terminal

; It is not a single char. Is it followed by a "?" char?

andt:	SUI	greatR+1
	JZ	prnt2		; Yes: Display the line number in decimal

; System Variable "Greater Than" subroutine.

; Pass a 16-bits value in BC to a machine language subroutine.

	INR	A		; Is it a ">" char?
	CZ	brkpnt		; Call RST 0 at 0000H

; "At the conclusion of the machine language subroutine,
; a Intel 8080 RET instruction returns control to VTL-2,
; and places the value found in the BC register pair into
; the question variable ">".

	MOV	M,C		; Copy BC to M(HL)
	INX	H
	MOV	M,B
	LXI	H,greater	; Point to ">" data storage
	MOV	A,M		; Get 1st byte from memory
	ADD	C		; Add C-reg
	MOV	C,A		; Move it to C-reg
	DCX	H		; Point to previous byte
	MOV	A,M		; Get 2nd byte from memory
	ADC	B		; Add B-reg
	MOV	M,C		; Move C-reg to ">" data storage
	INX	H		; Go back to 1st byte
	MOV	M,A		; Move B-reg to ">" data storage

; Compare BC with DE.

cpbd:	MOV	A,C		; BC = ">" data storage
	CMP	E
	RNZ

	MOV	A,B		; DE = ?
	CMP	D
	RET

;--------------------------------
; Skip what?

skp2:	CALL	find

	JNC	insrt

	CALL	lxhn		; Get line number from user program,
;				    and compare with ?

	JNZ	insrt		; Not zero: Insert

	CALL	nxtxt

	XCHG
	LHLD	number		; Lin ptr
delt:	CALL	cpbd		; Compare BC with DE

	JZ	fitit		; Equal: ?

; Not equal: ?

	LDAX	D		; Move byte at M(DE)
	MOV	M,A		;   to M(HL).
	INX	H		; Increment HL
	INX	D		; Increment DE
	JMP	delt		; Loop

; Insert a program line in program "text".

fitit:	SHLD	ampers		; Save address of next byte of user program
	MOV	B,H		; Copy HL to BC
	MOV	C,L
insrt:	LHLD	curent
	LXI	D,0003H		; 3 = line number + space?
	MOV	A,M		; Get byte
	ANA	A		; Is it zero?
	JZ	loop		; Yes: Back to main loop of interpreter

cntln:	INX	D		; No:
	INX	H
	MOV	A,M		; Get next byte
	ANA	A
	JNZ	cntln		; Not zero: Back to main loop of interpreter

; Zero: ?

	XCHG
	DAD	B		; Add BC
	XCHG			; Put result in DE
	LXI	H,asterix	; Memory size
	MOV	A,E		; Subtract memory size from program length?
	SUB	M
	INX	H
	MOV	A,D
	SBB	M
	JNC	start		; No memory: Return to interpreter

; Some memory is present.

	XCHG
	SHLD	ampers		; Then, this must be next byte of user PGM?
	INX	B
	INX	H
	PUSH	H
	LHLD	number		; Lin ptr
	XCHG
	POP	H

slide:	DCX	B
	DCX	H
	LDAX	B		; A = M(BC)
	MOV	M,A
	CALL	cpbd		; Compare ? with ?

	JNZ	slide		; Move program "text"

	LHLD	dollar		; Char ptr
	MOV	A,L		; Get M(HL)
	STAX	B		; M(BC) = A
	INX	B		; Increment BC
	MOV	A,H		; Get M(HL)
	STAX	B		; M(BC) = A
	LHLD	curent
	DCX	H		; Decrement it?

; Move one line in memory?

movl:	INX	H
	INX	B
	MOV	A,M
	STAX	B		; M(BC) = A
	ANA	A
	JNZ	movl		; Not zero: Loop

	JMP	loop		; Zero: Back to main loop of interpreter

;--------------------------------
; Print 16-bit line number in decimal.

prnt2:	LXI	D,decbuf	; Decimal buffer (5 chars long)
	LXI	H,pwrs10	; Table of decimal values
cvd1:	PUSH	D		; Save address of DECBUF on Stack
	MOV	D,B		; Copy BC to DE
	MOV	E,C
	MOV	C,M		; Get low byte of dec value
	INX	H
	MOV	B,M		; Get high byte of dec value
	INX	H
	PUSH	H		; Save address of next word
	XCHG
	CALL	div		; 16-bits division

	XCHG
	MOV	A,L		; Get low byte
	MOV	B,D		; Copy DE to BC
	MOV	C,E
	POP	H		; Restore address of next dec value
	POP	D		; Restore address of DECBUF
	ADI	zero		; Make it an ASCII number
	STAX	D		; M(DE) = A
	INX	D
	MOV	A,M		; Get high byte
	CPI	7EH		; "~" ?
	JNZ	cvd1		; No: Loop

; Yes: ?

	LXI	H,decbuf-1	; Point before decimal buffer
	DCX	D
	LDAX	D		; A = M(DE)
	ORI	10000000B	; Why?
	STAX	D		; M(DE) = A

; Suppress zeroes?

zrsup:	INX	H		; Point to next char
	MOV	A,M		; Get it
	CPI	zero		; Is it an ASCII zero?
	JZ	zrsup		; Yes: Loop, not showing zeroes

; Print message -- Init delimiter.

pntmsg: XRA	A		; Init delimiter

; Start displaying a message.

strtmsg:STA	delim		; Save delimiter
	MOV	B,A		;    and put it in B-reg.
outmsg: MOV	A,M		; Get char from memory
	INX	H		; Update memory pointer
	CMP	B		; Is it the delimiter?
	JZ	contC		; Yes: Check if it is Ctrl-C

	CALL	ascii		; No: Display the char on the terminal

	JMP	outmsg		;    and loop until delimiter found.

; Check for Control-C.

contC:	CALL	polcat		; Poll character at terminal?

	RNC			; None available: Return

	CALL	inch		; One char present: Get it

	CPI	ctrlC		; Is it Control-C?
	JZ	start		; Yes: Return to interpreter

	JMP	inch		; No: Get char

;--------------------------------
; Evaluate expressions between parentheses.

eval:	CALL	getval		; Get value of a number

; Next term of an expression?

nxtrm:	MOV	A,M
	ANA	A		; Number = zero?
	RZ			; Yes: Return

	CPI	Rparen		; No: Is it a closing parenthesis?
	JZ	outd		; Yes: INC HL and return

	CALL	term		; No: Process term of expression

	MOV	B,H		; Copy HL to BC
	MOV	C,L
	LHLD	evalptr		; Load eval pointer
	JMP	nxtrm		;    and loop until 00H found.

;--------------------------------
; Process one term of an arithmetic expression.

term:	PUSH	B		; Put BC on Stack
	MOV	A,M		; Get char
	PUSH	PSW		; Save it on Stack
	INX	H		; Point to next char
	CALL	getval		; Get value of a decimal number

	SHLD	evalptr		; Save eval pointer?
	POP	PSW		; Restore char
	POP	H		; Place BC into HL

; Is it followed by an addition?

	CPI	plus
	JNZ	eval2		; No: Check next arithmetic operation

; Yes: 16-bit addition.

	DAD	B		; Add BC to HL
	RET			; Result in HL

;--------------------------------
; Is it followed by a subtraction?

eval2:	CPI	minus
	JNZ	eval3		; No: Check next arithmetic operation

; Yes: 16-bit subtraction subroutine.

hsubb:	MOV	A,L		; Subtract BC from HL
	SUB	C
	MOV	L,A
	MOV	A,H
	SBB	B
	MOV	H,A
	RET			; Result in HL

;--------------------------------
; Is it followed by a multiplication?

eval3:	CPI	star
	JNZ	eval4		; No: Check next arithmetic operation

; Yes: 16-bit multiplication subroutine.

	XCHG			; Put multiplicand in DE
	LXI	H,0000H		; Clear partial product
	MVI	A,10H		; Set loop count to 16 bits
mult1:	PUSH	PSW		; Save it on Stack
	DAD	H		; Add to product
	XCHG			; Put it into DE
	DAD	H		; Add to product
	XCHG			; Put it into HL
	JNC	mult2		; If the result has 17 bits,

	DAD	B		;   add multiplier to product.
mult2:	POP	PSW		; Restore count
	DCR	A		; Decrement it
	JNZ	mult1		; Not end: Loop

	RET			; Result in HL

;--------------------------------
; Is it followed by a division?

eval4:	CPI	slash
	JNZ	eval5		; No: Now, check relational operators

	CALL	div		; Yes: Use a subroutine

	SHLD	percent		; Remainder of divide operation
	XCHG
	RET			; Result in HL

;--------------------------------
; 16-bit division subroutine.

; BC: divisor, DE: dividend, HL: remainder

div:	XCHG			; Put dividend in DE
	LXI	H,0000H		; Initialize remainder in HL
	MOV	A,B		; Checks if 16-bits divisor = zero
	ORA	C
	RZ			; Yes: Return (division by zero is forbidden)

	MVI	A,10H		; Set loop count to 16 bits
div1:	PUSH	PSW		; Save it on Stack
	DAD	H
	XCHG			; Dividend
	DAD	H		;   bit
	XCHG			;   to Carry.
	JNC	div2		; It was zero

	INX	H		; It was one, duplicate in HL
div2:	CALL	hsubb		; Subtract BC from HL

	INX	D
	JNC	div3		; It was zero

	DAD	B		; Add divisor to remainder
	DCX	D
div3:	POP	PSW		; Restore loop count
	DCR	A		; Decrement it
	JNZ	div1		; Not end: Loop

	RET

;--------------------------------
; Now, check the relational operators.

eval5:	LXI	D,0000H		; Init DE with FALSE value
	CALL	evil5		; Check the relational operators

	XCHG			; HL will contain the TRUE/FALSE value
	RET

;--------------------------------
; Is it followed by an equality?

evil5:	SUI	equal
	JNZ	evil6		; No: Check next relational operators

	CALL	hsubb		; Yes: Use a subroutine

	RNZ			; Return if not zero

	ORA	L		; Is it zero?
	RNZ			; No: Return if not zero (FALSE)

	INX	D		; Yes: = 1 (TRUE value)
	RET

;--------------------------------
; Is it followed by a greater than?

evil6:	DCR	A		; "=" - 1 = "<"
	JZ	evil7		; Yes: Check next relational operator

	CALL	hsubb		; No: Use a subroutine

	RNC			; Return if FALSE

	INX	D		; = 1 (TRUE value)
	RET

;--------------------------------
; Is it followed by a less than?

evil7:	CALL	hsubb		; Use a subroutine

	RC			; Return if FALSE

	INX	D		; = 1 (TRUE value)
	RET

;--------------------------------
; Get value of a decimal number.

getval: CALL	cvbin		; Convert to binary number

	RNC			; It was a number: Return

	CPI	quest		; Is it a PRINT statement?
	INX	H		; Increment mem ptr
	JNZ	var		; No: Check if it is a variable name

; Yes: ?

	SHLD	valvar		; Save value of variable?
	MVI	A,1		; A loaded script supplies the program, not the answers
	STA	askcon
	CALL	inln
	XRA	A
	STA	askcon

	CALL	eval		; Evaluate expression

	LHLD	valvar		; Value of expression?
	RET

;--------------------------------
; Is it a variable name?

var:	CPI	dolar		; Is it an INPUT statement?
	JNZ	var1		; No: Check next possibility

	CALL	inch		; Yes: Input next char

	MOV	C,A
	MVI	B,00H		; Make sure a char is a byte value
	RET

; Is it the start of an expression?

var1:	CPI	Lparen
	JZ	eval		; Yes: Evaluate expression

	DCX	H		; No: Re-point to character

var2:	CALL	convp

	MOV	C,M
	INX	H
	MOV	B,M
	LHLD	result
	RET

;--------------------------------
; Obviously, deal with array DEEK/DOKE...

array:	CALL	eval		; Evaluate expression

	SHLD	result		; Save result of evaluation?
	LHLD	ampers		; Get address of next byte of user program
	DAD	B
	DAD	B
	POP	PSW
	RET

;--------------------------------
; Convert p...?

convp:	MOV	A,M
	INX	H
	PUSH	PSW
	CPI	colon		; Is it the start of an array statement?
	JZ	array		; Yes: Go to ARRAY subroutine

; No: ?

	SHLD	result		; Save value of expression
	ANI	00111111B	; = 3FH
	ADD	A		; duplicate A
	ADI	vars AND 0FFH	; make sure that no carry occurs!
	MOV	L,A		; lower part
	MVI	H,vars SHR 8	; HL now points to variable
	POP	PSW
	RET

;--------------------------------
; Table of powers of 10.

pwrs10:	DW	10000
	DW	 1000
	DW	  100
	DW	   10
	DW	    1
	DB	7EH		; prnt2 end marker; was the MOV A,M opcode of tstn

;--------------------------------
; Test if it is an ASCII decimal number character.

tstn:	MOV	A,M		; Get char
	CPI	'9'+1		; Is it > "9" ?
	CMC
	RC
	CPI	'0'		; Is it < "0" ?
	RET

;--------------------------------
; Convert to line number.

cvtln:	CALL	inln

; Convert to binary number.

cvbin:	CALL	tstn		; Is it a number?

	RC			; No: Return

; Yes: Convert the ASCII char to binary number.

	LXI	B,0000H
cbloop: MOV	A,M		; Get char
	SUI	zero		; Convert from ASCII to binary
	ADD	C		; Add C-reg
	MOV	C,A		; Put result in C-reg
	MVI	A,00H		; Re-init A-reg
	ADC	B		; Add B-reg
	MOV	B,A		; Put result in B-reg
	INX	H		; Point to next char
	CALL	tstn		; Is it a number?

	CMC
	RNC			; No: Return

; Yes: ?

	PUSH	H		; Save pointer
	MOV	H,B		; Copy BC to HL
	MOV	L,C
	DAD	H		; 2 times HL
	DAD	H		; 4 times HL
	DAD	B		; 2 times BC
	DAD	H		; 8 times HL
	MOV	B,H		; Move HL to BC ?
	MOV	C,L
	POP	H		; Restore pointer
	JMP	cbloop		; Loop until not a number

;--------------------------------
; Process line input from terminal.

inln6:	CPI	atsign		; Discard line inputted?
	JZ	newlin		; Yes: Start a new line

; No: We are in a line. Are we at the end?

	INX	H		; Increment line pointer
	MOV	A,L		; Get its low byte
	CPI	(linbuf+bufsiz) AND 0FFH	; Are we at the end of the line?
	JNZ	inln2		; No: Get next char

; Yes: Start a new line on the terminal.

newlin: CALL	crlf		; Return "cursor" at beginning of next line

inln:	LXI	H,linbuf+1	; Point to first char in line buffer

; Check if we reached the beginning of the line,
; that is to say: erased all the line, backspacing chars.

inln5:	DCX	H		; Point before the first char
	MOV	A,L
	CPI	(linbuf-1) AND 0FFH	; Is it the end of the line?
	JNZ	inln2		; No: Get another char
	INX	H		; Rubout on an empty line: stay where we are

; Not at end of line: Get another char, and check it.

inln2:	CALL	getchr		; Input another char (console or script)

	MOV	M,A
	CPI	backs		; Backspace to erase a char?
	JZ	inln3		; Yes: Rub the char off the screen
	CPI	rubout		; Terminals usually send DEL for Backspace
	JZ	inln3
	NOP

; No: Check if the user had ended inputting a line.

	CPI	lf		; A bare LF ends a line too
	JZ	inln8
	CPI	cr
	JC	inln2		; No: Get another char

	JNZ	inln6		; Yes: Go proceed another line

	XRA	A		; Mark end of line with a 00H
	MOV	M,A
	LXI	H,linbuf	; Why?
	LDA	askcon		; Console input echoed the CR, so it needs the LF
	ORA	A
	JNZ	inln9
	LDA	scriptflg	; Script input has no echoed CR to complete,
	ORA	A
	RNZ			;   so the LF would leave a blank line

; The terminal echoed the CR. If it sent CR/LF, eat the LF unechoed,
; otherwise supply the LF ourselves.

inln9:	CALL	polcat
	JNC	prlf
	CALL	rawin
	CPI	lf
	RZ
	JMP	prlf

;--------------------------------
; A bare LF already moved the cursor down, so no CR/LF is added here.
; An LF on an empty line is just the tail of a CR/LF pair: discard it.

inln8:	MOV	A,L
	CPI	linbuf AND 0FFH
	JZ	inln2
	XRA	A
	MOV	M,A
	LXI	H,linbuf
	RET

;--------------------------------
; Wipe the erased char off the screen, then go check if the line is gone.

inln3:	MVI	A,' '
	CALL	outch
	MVI	A,backs
	CALL	outch
	JMP	inln5

;--------------------------------

quote2: INX	H		; Skip beginning quote

strng:	CALL	strtmsg		; Proceed start of string

	MOV	A,M		; Get char
	CPI	semicol		; Is it a semicolon?
	RZ			; Yes: Then, no cr/lf after PRINT

; No: End of line reached, output CR and LF.

crlf:	MVI	A,cr		; Carriage Return
	CALL	outch

prlf:	MVI	A,lf		; Line Feed
	CALL	outch
	RET

;--------------------------------

banner:	DB	'VTL2 Interpreter 1.0', 00H

okm:	DB	cr, lf
	DB	'OK', 00H

	; carry set iff character is available
polcat:	PUSH	H
	PUSH	D
	PUSH	B
	MVI	C,CONSTAT	; Get console status
	CALL	BDOS		; A=0FFH if character available else A=0
	RRC
	JMP	done

	; return the next character in A
inch:	PUSH	H
	PUSH	D
	PUSH	B
	MVI	C,CONIN		; Console input
	CALL	BDOS
	ANI	7FH
	JMP	done

	; return the next character in A without echoing it
rawin:	PUSH	H
	PUSH	D
	PUSH	B
	MVI	E,0FFH
	MVI	C,DIRCON
	CALL	BDOS
	ANI	7FH
	JMP	done

	; Output character in A, optionally clearing highest bit
ascii:	ANI	7FH
outch:	PUSH	H
	PUSH	D
	PUSH	B
	PUSH	PSW
	MOV	E,A
	MVI	C,CONOUT	; Console output
	CALL	BDOS
	POP	PSW
done:	POP	B
	POP	D
	POP	H
	RET

;--------------------------------
; Get next char from script file if active, else console.

; Caller does MOV M,A with HL pointing into linbuf, so this must preserve
; HL/DE/BC exactly like inch does.

getchr:	PUSH	H
	PUSH	D
	PUSH	B
	LDA	askcon
	ORA	A
	JNZ	getcon
	LDA	scriptflg
	ORA	A
	JZ	getcon
	CALL	scrget
	JMP	getdon
getcon:	CALL	inch
getdon:	POP	B
	POP	D
	POP	H
	RET

scrget:	LDA	scriptptr
	MOV	B,A
	LDA	scriptlen
	CMP	B
	JNZ	scrbyt
	CALL	scrrfl
	LDA	scriptflg
	ORA	A
	JZ	inch
scrbyt:	LXI	H,scrbuf
	LDA	scriptptr
	MOV	E,A
	MVI	D,0
	DAD	D
	MOV	A,M
	ANI	7FH		; Mask high bit, same as inch does for console input
	CPI	lf
	JNZ	scrnlf
	LDA	scriptptr
	INR	A
	STA	scriptptr
	JMP	scrget
scrnlf:	CPI	1AH
	JZ	scrend
	PUSH	PSW
	LDA	scriptptr
	INR	A
	STA	scriptptr
	POP	PSW
	RET

scrend:	CALL	screof
	JMP	inch

;--------------------------------
; Open script file from default FCB, if a name is present.
; Runs only once: `start` is re-entered on every REPL bounce, but the
; script file must be opened exactly once, at true cold start.

scrini:	LDA	scrdid		; Already initialized once?
	ORA	A
	RNZ			; Yes: leave scriptflg/ptr/len untouched
	MVI	A,1
	STA	scrdid
	MVI	A,0
	STA	scriptflg
	STA	scriptptr
	STA	scriptlen
	LDA	0080H		; Command tail length (0 = no argument given)
	ORA	A
	RZ			; No argument at all: skip file logic entirely
	LXI	H,FCB+1
	MOV	A,M
	ORA	A
	JZ	scrdon
	CPI	' '
	JZ	scrdon
	LXI	D,FCB
	MVI	C,OPEN
	CALL	BDOS
	CPI	0FFH
	JZ	scrdon
	MVI	A,1
	STA	scriptflg
	STA	skipok
scrdon:	RET

;--------------------------------
; Refill script buffer from file.

scrrfl:	LXI	D,DMA
	MVI	C,SETDMA
	CALL	BDOS
	LXI	D,FCB
	MVI	C,READ
	CALL	BDOS
	ORA	A
	JNZ	screof
	LXI	H,DMA
	LXI	D,scrbuf
	MVI	B,80H
scrcpy:	MOV	A,M
	STAX	D
	INX	H
	INX	D
	DCR	B
	JNZ	scrcpy
	MVI	A,0
	STA	scriptptr
	MVI	A,80H
	STA	scriptlen
	RET

screof:	MVI	A,0
	STA	scriptflg
	STA	scriptptr
	STA	scriptlen
	RET

; convp indexes this block with an 8-bit add and drops the carry, so vars
; must start on a page boundary for offsets 0..126 to stay in range.

	ORG	0800H

vars:	DS	62		; @, A - Z
number:	DS	4
exclam:	DS	4		; !
dollar:	DS	4		; $
percent:DS	2		; %
ampers:	DS	3		; &
greater:DS	5		; >
asterix:DS	12		; *
evalptr:DS	12
result:	DS	2
linumb:	DS	2
curent:	DS	2
valvar:	DS	12

scriptflg:	DB	0
scriptptr:	DB	0
scriptlen:	DB	0
scrdid:	DB	0
skipok:	DB	0
askcon:	DB	0

scrbuf:	DS	128
decbuf:	DS	4+1
delim:	DS	1
linbuf:	DS	bufsiz

prgm	EQU	$

	END
