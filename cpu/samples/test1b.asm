;
;	8080 assembler test instruction file
;
	ORG	100H			; Test org pseudo op
LABEL1:	EQU	$			; Test equ pseudo op
SYMBOL:	DW	LABEL1			; Test defw pseudo op
SYMBOL:	DB	03FH			; Test defb pseudo op form 1
SYMBOL	DB	'THIS IS A TEST'	; Test defb pseudo op form 2
SYMBOL:	DS	55			; Test defs pseudo op
;
	IBM	'ASCII FIRST,0DFF,THEN HEX
;
	MOV	A,B			; Test load
	MVI	A,0FFH
	MOV	A,M
	MOV	B,C			; Test load
	MVI	B,0FFH
	MOV	B,M
	MOV	M,B
	MOV	M,M
	MVI	M,23
	STAX	B
	STAX	D
	STA	LABEL1
	LDAX	B
	LDAX	D
	LDA	LABEL1

	LXI	B,LABEL1
	LXI	D,LABEL1
	LXI	H,LABEL1
	LXI	SP,LABEL1

	LHLD	LABEL1
	SHLD	LABEL1

	SPHL

	PUSH	B
	PUSH	D
	PUSH	H
	PUSH	PSW

	POP	B
	POP	D
	POP	H
	POP	PSW

	XCHG
	XTHL

	ADD	A
	ADI	0FFH
	ADD	M
	ADC	A
	ACI	0FFH
	ADC	M
	SUB	A
	SUI	0FFH
	SUB	M
	SBB	A
	ANA	A
	ANI	0FFH
	ANA	M
	ORA	A
	XRA	A
	CMP	A

	INR	A
	INR	M
	DCR	M

	DAD	B

	INX	B
	INX	D
	INX	H
	INX	SP

	DCX	B
	DCX	D
	DCX	H
	DCX	SP

	RLC
	RAL
	RRC
	RAR

	DAA
	CMA
	CMC
	STC
	NOP
	HLT
	DI
	EI

	JMP	LABEL1
	JNZ	LABEL1
	JZ	LABEL1
	JNC	LABEL1
	JC	LABEL1
	JPO	LABEL1
	JPE	LABEL1
	JP	LABEL1
	JM	LABEL1

	PCHL

	CALL	LABEL1
	CNZ	LABEL1
	CZ	LABEL1
	CPO	LABEL1
	CPE	LABEL1

	RET
	RNZ
	RZ
	RPO

	RST	7

	IN	03H
	OUT	034H

LABEL	JNZ	$+3			; Test !label2 jmp label ; comment ! lb3:rpo
;***********************************************************************
;***********************************************************************
;

FILE:	XTHL				; Comment 17
	XCHG				; Comment 25
					; Comment 25
	LONGNAME			; Comment 33
LONGNAME				; Comment 41
LONGNAME: :
	XRA	A			; Comment 25
LONGNAME: :CMA				; Comment 25
LONGNAME: :	CMA
;
ELAB:	:	LHLD	NNNNNNNNNNNNN	; Comment 33
LONGNAME: CMA
LONGNAME: CMA
SHORT1:	:CMA				; Comment
SHORT:					; Comment 9
SHORT					; Comment 9
					; Comment 9
LABEL	CMA!CMA! CMA			; Comment 25
LABEL	CMA	!CMA !CMA		; Comment 25
LABEL	CMA	! CMA !	CMA		; Comment 25
	DAA				; Comment 17
	END				; Comment 17

	END	$
