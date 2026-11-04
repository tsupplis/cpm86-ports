; w o r k h o r s e   r o u t i n e s
; inline print of message
ILPRT:
        MOV    BP,SP                   ;save hl, get msg pointer.
        XCHG   BX,[BP]
        MOV    AL,27
        CALL   TTYPE
        MOV    AL,'b'
        CALL   TTYPE
        MOV    AL,10
        CALL   TTYPE
ILPLP:
        MOV    AL,M                    ;get character
        INC    BX                      ;pt to next
        AND    AL,7FH                  ;strip type bits
        JZ     ILPLP1
        CALL   TTYPE                   ;show on console
        JMPS   ILPLP
ILPLP1:
        MOV    BP,SP                   ;set hl-pair and..
        XCHG   BX,[BP]
        MOV    AL,27
        CALL   TTYPE
        MOV    AL,'b'
        CALL   TTYPE
        MOV    AL,15
        CALL   TTYPE
        RET                            ;..return past message.
; output 'crlf' to console
CRLF:
        MOV    AL,CR
        CALL   TTYPE
        MOV    AL,LF
; conout routine
TTYPE:
        LAHF
        XCHG   AL,AH
        PUSH   AX
        XCHG   AL,AH
        PUSH   CX
        PUSH   DX
        PUSH   BX
        LAHF                           ; check for flow control
        XCHG   AL,AH
        PUSH   AX
        XCHG   AL,AH
        CALL   CST                     ; BIOS console status
        OR     AL,AL                   ; 0 means nothing
        JZ     TYPE1
        CALL   CIN                     ; BIOS console input
        CMP    AL,CTRLS                ; pause?
        JNZ    TYPE1
        CALL   CIN                     ; BIOS console input
TYPE1:
        POP    AX                      ; get char
        XCHG   AL,AH
        MOV    DL,AL
        MOV    CL,WRCON
        INT    224
        POP    BX
        POP    DX
        POP    CX
        POP    AX
        XCHG   AL,AH
        SAHF
        RET

; direct I/O
CST:   ;Console status
        PUSH   DX
        PUSH   BX
        MOV    DL,0FFH
        MOV    CL,DIRCON
        INT    224
        POP    BX
        POP    DX
        RET

CIN:   ;Input from console - no echo   (getchar)
        PUSH   DX
        PUSH   BX
        PUSH   CX

; Use BDOS on both systems. CP/M-86 sends VT52 sequences, whereas
; concurrent XIOS sends high-bit scan codes with PFK expansion disabled.
        CALL    CINRAW
        CMP     Byte Ptr CCKEYS,0
        JE      CINESCP
        CMP     AL,0C7H                ;Home..Delete, including all arrows
        JB      CINESCP
        CMP     AL,0D3H
        JA      CINESCP
        MOV     AH,AL
        AND     AH,7FH                 ;scan code expected by FTABLE
        XOR     AL,AL
        JMP     CIN4
CINESCP:
        CMP     AL,ESC
        JNZ     CIN3                    ;ordinary key
        CALL    CINRAW                  ;second byte selects the cursor key
        MOV     BX,(Offset ESCTAB)
CIN1:
        MOV     AH,M
        OR      AH,AH
        JZ      CIN5                    ;not a cursor key, so ESC as typed
        CMP     AH,AL
        JZ      CIN2
        INC     BX
        INC     BX
        JMPS    CIN1
CIN2:
        INC     BX
        MOV     AH,M                    ;scan code for the FTABLE lookup
        MOV     AL,0
        JMPS    CIN4
CIN5:
        MOV     AL,ESC
CIN3:
        MOV     AH,0
CIN4:
        MOV     Byte Ptr CHR_LO, al ; ah=0      if fkey or keypad  
        MOV     Byte Ptr CHR_HI, ah ; ah=chrin  if fkey or keybad  

        POP     CX
        POP     BX
        POP     DX
        RET

KEYINIT:
        MOV     CL,12
        INT     224
        CMP     AL,30H
        JB      KEYINITDONE
        TEST    AH,5
        JZ      KEYINITDONE
        MOV     Byte Ptr CCKEYS,1
        CALL    KEYON
KEYINITDONE:
        RET

; These helpers preserve the command-chain registers (including CX/SI/DI).
KEYON:
        PUSH    AX
        MOV     AL,'6'
        JMP     KEYMODE
KEYOFF:
        PUSH    AX
        MOV     AL,'7'
KEYMODE:
        CMP     Byte Ptr CCKEYS,0
        JE      KEYMODEDONE
        PUSH    AX
        MOV     AL,ESC
        CALL    TTYPE
        POP     AX
        CALL    TTYPE
KEYMODEDONE:
        POP     AX
        RET

CINRAW:                                 ;wait for one byte, no echo
        MOV     DL,0FFH
        MOV     CL,DIRCON
        INT     224
        OR      AL,AL
        JZ      CINRAW
        RET

; output 'crlf' to printer
LCRLF:
        MOV    AL,CR
        CALL   LOUT
        MOV    AL,LF
; printer routine
LOUT:
        LAHF
        XCHG   AL,AH
        PUSH   AX
        XCHG   AL,AH
        PUSH   CX
        PUSH   DX
        PUSH   BX
        MOV    DL,AL
        MOV    CL,LLIST
        INT    224
        POP    BX
        POP    DX
        POP    CX
        POP    AX
        XCHG   AL,AH
        SAHF
        RET
; crt clear-line function
CLR@L:
        MOV    AL,CR
        CALL   TTYPE
        MOV    CH,30                   ;blank # of characters on line
        MOV    AL,' '
CL@LP:
        CALL   TTYPE
        DEC    CH
        JNZ    CL@LP
        RET
; conin routine (waits for response)
KEYIN:
        MOV    CL,RDCON
        INT    224
; convert character in a-reg to upper case
UCASE:
        CMP    AL,61H                  ;less than small 'a'?
        JNB    L_C23   
        RET                            ;if so, no convert needed.
L_C23:
        CMP    AL,7AH+1                ; >small 'z'?
        JNAE   L_C24   
        RET                            ;if so, ignore.
L_C24:
        AND    AL,5FH                  ;otherwise convert
        RET
; direct console input w/o echo (waits for input)
DKEYIN:
        CALL   CIN                     ;get char from BIOS
        AND    AL,7FH                  ;mask MSB
        JMPS   UCASE                   ;capitalize
; convert keyboard input to upper case
CONVERT:
        MOV    BX,(Offset CMDBUF)+1    ; 'current keyboard buffer length'..
        MOV    CH,M                    ;..to b-reg.
        MOV    AL,CH
        OR     AL,AL                   ;if zero length, skip conversion.
        JNZ    L_C25   
        JMP    COMCAN
L_C25:
CONVLP:
        LAHF                           ;point at character to capitalize
        INC    BX
        SAHF
        MOV    AL,M
        CALL   UCASE
        MOV    M,AL                    ;put back into buffer
        DEC    CH
        JNZ    CONVLP
        RET
; fill buffer with 'spaces' with count in b-reg
FILL:
        MOV    M,' '                   ;put in space character
        LAHF
        INC    BX
        SAHF
        DEC    CH
        JNZ    FILL                    ;no, branch.
        RET
; ignore leading spaces (ls) in buffer, length in c-reg.
UNSPACE:
        MOV    SI,DX                   ;get character
        MOV    AL,[SI]
        CMP    AL,' '
        JZ     L_C26   
        RET                            ;not blank, a file is entered.
L_C26:
        INC    DX                      ;to next character
        DEC    CL
        JNZ    L_C27   
        JMP    COMCAN                  ;all spaces --> command recovery error
L_C27:
        JMPS   UNSPACE
; check for legal cp/m filename character -- return with carry set if illegal
CKLEGAL:
        MOV    SI,DX                   ;get character from de-pair
        MOV    AL,[SI]
        INC    DX                      ;point at next character
        CMP    AL,' '                  ;less than space?
        JNB    L_C28   
        RET                            ;return carry if unpermitted character
L_C28:
        PUSH   BX
        PUSH   CX
        CMP    AL,'['                  ;if greater than 'z', exit with..
        JNB    CKERR                   ;..carry set.
        MOV    CH,(Offset CHR@TEND)-(Offset CHR@TBL)
        MOV    BX,(Offset CHR@TBL)
CHR@LP:
        CMP    AL,M
        JZ     CKERR
        INC    BX
        DEC    CH
        JNZ    CHR@LP
        OR     AL,AL                   ;clear carry for good character
        POP    CX
        POP    BX
        RET
CKERR:
        POP    CX
        POP    BX
        STC                            ;error exit with carry set
        RET
L_C29  EQU     $
        DSEG
        ORG    Offset L_C29
CHR@TBL        RS      0
        DB     ',',':',';','<','=','>' ;invalid character table
L_C30  EQU     $
        CSEG
        ORG    Offset L_C30
CHR@TEND:
        RS     0
; print file name in S$FCB
PRFNSX:
        PUSH   BX                      ;save regs
        PUSH   CX
        JMPS   PRFNS0
PRFNS:
        PUSH   BX                      ;affect only PSW
        PUSH   CX
        MOV    BX,(Offset S@FCB)+1
PRFNS0:
        CALL   PRFN                    ;print file name
        POP    CX                      ;restore
        POP    BX
        RET
; print file name pted to by HL
PRFN:
        MOV    CH,8                    ;8 chars
        CALL   PRFNS1
        MOV    AL,'.'
        CALL   TTYPE
        MOV    CH,3                    ;file type and fall thru
PRFNS1:
        MOV    AL,M                    ;get char
        CALL   TTYPE
        LAHF                           ;pt to next
        INC    BX
        SAHF
        DEC    CH
        JNZ    PRFNS1
        RET

; print filename  & tag in reverse video if at cursor
AC@PRFN:
IF     ENH@VID
        CALL   CUR@VID                 ;set reverse video 
        CALL   PRFN                    ;print file name
        CMP    M,'*'                   ;is it tagged?
        JNE    AC@RET
        MOV    AL,M                    ;print tag
        CALL   TTYPE
AC@RET:
        MOV    AL,' '                  ;*added -ken
        CALL   TTYPE                   ;*added -ken
        CALL   STD@VID
        RET
ENDIF  ;ENH@VID

IF  NOT ENH@VID
        CALL   PRFN
        MOV    AL,M                    ;print tag
        JMP    TTYPE
ENDIF  ;NOT ENH@VID

; print filename if not at  cursor
NC@PRFN:
        CMP    Byte Ptr .11[BX],'*'    ;is it tagged?
        JNZ    STD                     ;use std video if not tagged
        CALL   TAG@VID
        JMPS   NO@STD
STD:    
        CALL   STD@VID
NO@STD: 
        CALL   PRFN                    ;print file name
        MOV    AL,M
        CALL   TTYPE
        JMP    STD@VID

; filename from 'ring' to 'sfcb'
RINGFCB:
        MOV    BX,Word Ptr RINGPOS     ;move name from ring to source 'fcb'
        MOV    DX,(Offset S@FCB)       ;place to move filename and..
        MOV    CH,12                   ;..amount to move.
; move subroutine -- move b-reg # of bytes from hl-pair to de-pair
MOVE:
        MOV    AL,M                    ;get hl-pair referenced source byte
        AND    AL,7FH                  ;strip cp/m 2.x attributes
        MOV    SI,DX                   ;put to de-pair referenced destination
        MOV    [SI],AL
        LAHF                           ;fix pointers for next search
        INC    BX
        SAHF
        LAHF
        INC    DX
        SAHF
        DEC    CH
        JNZ    MOVE
        RET
; initialize 'fcb' cp/m system fields (entry with hl-pair pointing to 'fcb')
INITFCB:
        MOV    CH,4                    ;fill ex, s1, s2, rc counters with zeros.
INITLP:
        MOV    M,0                     ;put zero (null) in memory
        LAHF
        INC    BX
        SAHF
        DEC    CH
        JNZ    INITLP
        RET
; disk system reset -- login requested drive
RESET:
        MOV    CL,INQDISK              ;determine and..
        INT    224                     ;..save..
        MOV    Byte Ptr C@DR,AL        ;..current drive.
        MOV    CL,RESETDK              ;reset system
        INT    224
        MOV    AL,Byte Ptr R@DR        ;make requested drive..
SET@DR:
        MOV    DL,AL                   ;..current.
        MOV    CL,LOGIN
        INT    224                     ;return to caller
        RET
; set/reset (or get) user area (call with binary user area in a-reg)
SET@USR:
        MOV    DL,AL                   ; 0 --> 0, 1 --> 1, etc.
GET@USR:
        MOV    CL,SGUSER
        INT    224                     ;return to caller
        RET
; compare de-pair to hl-pair and set flags accordingly
CMPDEHL:
        MOV    AL,DH                   ;see if high bytes set flags
        CMP    AL,BH
        JZ     L_C31   
        RET                            ;return if not equal
L_C31:
        MOV    AL,DL
        CMP    AL,BL                   ;low bytes set flags instead
        RET
; shift hl-pair b-reg bits (-1) to right (divider routine)
SHIFTLP:
        DEC    CH
        JNZ    L_C32
        RET
L_C32:
        MOV    AL,BH
        OR     AL,AL
        RCR    AL,1
        MOV    BH,AL
        MOV    AL,BL
        RCR    AL,1
        MOV    BL,AL
        JMPS   SHIFTLP
; decimal pretty print (h-reg contains msb; l-reg, the lsb.)
DECOUT:
        MOV    AL,5                    ;set leading space count
        MOV    Byte Ptr LDSP,AL
DECOU1:
        LAHF
        XCHG   AL,AH
        PUSH   AX
        PUSH   CX
        PUSH   DX
        PUSH   BX
        MOV    AL,Byte Ptr LDSP        ;count down
        DEC    AL
        MOV    Byte Ptr LDSP,AL
        MOV    CX,-10                  ;radix
        MOV    DX,-1
DECOU2:
        ADD    BX,CX                   ;sets..
        INC    DX
        JB     DECOU2                  ;..carry.       
        MOV    CX,10
        ADD    BX,CX
        XCHG   BX,DX
        MOV    AL,BH
        OR     AL,BL
        JZ     L_C33   
        CALL   DECOU1                  ; (recursive)
L_C33:
        MOV    AL,Byte Ptr LDSP        ; any spaces?
        OR     AL,AL                   ; 0=none
        JZ     DECOU4
        MOV    CH,AL                   ; count in B
        MOV    AL,' '
DECOU3:
        CALL   TTYPE
        DEC    CH
        JNZ    DECOU3
        XOR    AL,AL                   ;A=0
        MOV    Byte Ptr LDSP,AL        ;set flag
DECOU4:
        MOV    AL,DL
        ADD    AL,'0'                  ;make ascii
        CALL   TTYPE
        POP    BX
        POP    DX
        POP    CX
        POP    AX
        XCHG   AL,AH
        SAHF
        RET
; determine free storage remaining on selected drive
FRESTOR:
        MOV    CL,INQDISK              ;determine current drive
        INT    224                     ;returns 0 as a:, 1 as b:, etc.
        INC    AL                      ;make 1 --> a:, 2 --> b:, etc.
        MOV    Byte Ptr .FCB,AL
        MOV    CL,GETPARM              ;current disk parameter block
        INT    224
        INC    BX                      ;bump to..
        INC    BX
        MOV    AL,ES:M                 ;..block shift factor.
        MOV    Byte Ptr BSHIFTF,AL     ; 'bsh'
        INC    BX                      ;bump to..
        MOV    AL,ES:M                 ;..block mask.
        MOV    Byte Ptr B@MASK,AL      ; 'blm'
        INC    BX                      ;bump to..
        INC    BX                      ;..get..
        MOV    DL,ES:M                 ;..maximum block number..
        INC    BX                      ;..double..
        MOV    DH,ES:M                 ;..byte.
        XCHG   BX,DX
        MOV    Word Ptr B@MAX,BX       ; 'dsm'
        MOV    CL,INQALC               ;address of cp/m allocation vector
        INT    224
        XCHG   BX,DX                   ;get its length
        MOV    BX,Word Ptr B@MAX
        INC    BX
        MOV    CX,0                    ;initialize block count to zero
GSPBYT:
        PUSH   DX                      ;save allocation address
        MOV    SI,DX
        MOV    AL,ES:[SI]
        MOV    DL,8                    ;set to process 8 bits (blocks)
GSPLUP:
        RCL    AL,1                    ;test bit
        JB     NOT@FRE
        INC    CX
NOT@FRE:
        MOV    DH,AL                   ;save bits
        DEC    BX
        MOV    AL,BL
        OR     AL,BH
        JZ     END@ALC                 ;quit if out of blocks
        MOV    AL,DH                   ;restore bits
        DEC    DL                      ;count down 8 bits
        JNZ    GSPLUP                  ;branch to do another bit
        POP    DX                      ;bump to next count..
        INC    DX                      ;..of allocation vector.
        JMPS   GSPBYT                  ;process it
END@ALC:
        POP    DX                      ;clear alloc vector pointer from stack
        MOV    BL,CL                   ;copy # blocks to hl-pair
        MOV    BH,CH
        MOV    AL,Byte Ptr BSHIFTF     ;get block shift factor
        SUB    AL,3                    ;convert from sectors to thousands (k)
        JZ     PRT@FRE                 ;skip shifts if 1k blocks
FREK@LP:
        SHL    BX,1                    ;multiply blocks by k-bytes per block
        DEC    AL                      ;multiply by 2, 4, 8, or 16.
        JNZ    FREK@LP
PRT@FRE:
        MOV    Word Ptr DISKSP,BX      ;save disk space
        RET
;
;  Print free space on disk
;
PRINT@FRE:
        CALL   ERMSG                   ;position and set flags
        DB     0
        MOV    BX,Word Ptr DISKSP
        CALL   DECOUT                  ; # of free k-bytes in hl-pair
        CALL   ILPRT
        DB     'K Bytes on Disk',0
        RET
;
;
;
;
;
