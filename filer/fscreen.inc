ZDNFIND:
        PUSH   DX                      ; SAVE DE
        MOV    Word Ptr DIRNAME,BX     ; SAVE DIRECTORY NAME AWAY
        MOV    CL,INQDISK              ; SAVE CURRENT POSITION
        INT    224
        MOV    Byte Ptr DISK,AL
        MOV    Byte Ptr T@DR,AL
        MOV    DL,GET
        CALL   GET@USR
        MOV    Byte Ptr USER,AL
        MOV    BX,Word Ptr DIRNAME     ; PT TO NAME
        JMPS   SVDISK                  ; CHECK DU FORM FIRST
;
;  LOOK AT START OF DU: FORM
;      ON ENTRY, HL PTS TO FIRST CHAR OF DIRECTORY NAME
;
SVDISK:
        MOV    CL,Byte Ptr MDISK       ; GET MAX DISK
;      INC     CL                      ; +1 FOR LATER COMPARE
        MOV    AL,M                    ; GET DISK LETTER
        CMP    AL,'A'                  ; DIGIT?
        JB     USERCK                  ; IF NO DIGIT, MUST BE USER OR COLON
        SUB    AL,'A'                  ; CONVERT TO NUMBER
        CMP    AL,CL                   ; LIMIT?
        JNAE   L_D3    
        MOV    CL,MDRIVE
        DEC    CL
        CMP    CL,AL
        JE     L_D3
        JMP    DIRNXX                  ; NAME IF OUT OF LIMIT
L_D3:
        MOV    Byte Ptr DISK,AL        ; SAVE DISK
        INC    BX                      ; PT TO NEXT CHAR
;
;  CHECK FOR USER
;
USERCK:
        MOV    AL,M                    ; GET POSSIBLE USER NUMBER
        CMP    AL,':'                  ; NO USER NUMBER
        JZ     DIRNX                   ; EXIT IF SO
        CMP    AL,' '                  ; NO USER NUMBER
        JZ     DIRNX
        OR     AL,AL
        JZ     DIRNX
        XOR    AL,AL                   ; ZERO USER NUMBER
        MOV    CH,AL                   ; B=ACCUMULATOR FOR USER NUMBER
USRLOOP:
        MOV    AL,M                    ; GET DIGIT
        INC    BX                      ; PT TO NEXT
        CMP    AL,':'                  ; DONE?
        JZ     USRDN
        CMP    AL,' '                  ; DONE?
        JZ     USRDN
        SUB    AL,'0'                  ; CONVERT TO BINARY
        JB     DIRNXX                  ; NAME IF USER NUMBER ERROR
        CMP    AL,10
        JNB    DIRNXX
        MOV    CL,AL                   ; NEXT DIGIT IN C
        MOV    AL,CH                   ; OLD NUMBER IN A
        ADD    AL,AL                   ; *2
        ADD    AL,AL                   ; *4
        ADD    AL,CH                   ; *5
        ADD    AL,AL                   ; *10
        ADD    AL,CL                   ; *10+NEW DIGIT
        MOV    CH,AL                   ; RESULT IN B
        JMPS   USRLOOP
USRDN:
        MOV    AL,CH                   ; GET NEW USER NUMBER
        CMP    AL,32                   ; WITHIN RANGE?
        JNB    DIRNXX                  ; NAME IF OUT OF RANGE
        MOV    Byte Ptr USER,AL        ; SAVE IN FLAG
;
;  VALID EXIT -- FOUND IT, SO LOAD BC AND EXIT FLAG; ON ENTRY, HL PTS TO :
;
DIRNX:
        MOV    AL,Byte Ptr USER        ; RETURN USER IN C, DISK IN B
        MOV    CL,AL
        MOV    AL,Byte Ptr DISK
        MOV    CH,AL
        MOV    AL,0FFH                 ; SET NO ERROR
        OR     AL,AL                   ; SET FLAGS
        POP    DX                      ; RESTORE DE
        RET
;
;  INVALID EXIT -- NOT FOUND OR ERROR
;      NO VALID RETURN PARAMETERS (BC, HL)
;
DIRNERR:
        XOR    AL,AL                   ; ERROR CODE
        MOV    Byte Ptr DNLOAD,AL      ; SET LOAD FLAG TO NO LOAD
        POP    DX                      ; RESTORE DE
        RET
DIRNXX:
        POP    DX
        XOR    AL,AL
        RET

;
;  OPEN FILE FOR GET
;
FIOPEN:
        PUSH   DX
        MOV    CL,OPEN                 ; OPEN FILE
        INT    224
        POP    DX
FIO1:
        MOV    CL,READ                 ; READ FIRST BLOCK
        INT    224
        MOV    BX,TBUF                 ; SET PTR
        MOV    Word Ptr FIPTR,BX
        RET
;
;  GET NEXT BYTE FROM FILE
;
FGET:
        PUSH   BX                      ; SAVE REGS
        PUSH   DX
        PUSH   CX
        MOV    BX,Word Ptr FIPTR       ; PT TO NEXT CHAR
        MOV    AL,M                    ; GET IT
        MOV    Byte Ptr FICHAR,AL      ; SAVE IT
        LAHF                           ; PT TO NEXT
        INC    BX
        SAHF
        MOV    Word Ptr FIPTR,BX       ; SET PTR
        MOV    DX,TBUF+80H             ; END OF BUFFER?
        CALL   CMPDEHL                 ; COMPARE
        JNZ    FGETD                   ; DONE IF NOT
        MOV    DX,(Offset S@FCB)       ; PT TO FCB
        CALL   FIO1                    ; READ BLOCK AND SET PTR
        OR     AL,AL                   ; SET FLAG (NZ = ERROR)
        JMPS   FGETD1
FGETD:
        XOR    AL,AL                   ; NO ERROR (Z)
FGETD1:
        POP    CX                      ; GET REGS
        POP    DX
        POP    BX
        MOV    AL,Byte Ptr FICHAR      ; GET CHAR
        RET
; message routines
; print VFILER banner
BANNER:
        CALL   CLS                     ;clear screen
        MOV    BX,0*256+0
        CALL   GOTOXY
        CALL   RAWPRT                  ;top border
        DB     27,'b',7,0DAH
        DB     0C4H,0C4H,0C4H,0C4H,0C4H,0C4H,0C4H,0C4H,0C4H,0C4H,0C4H,0C4H,0C4H
        DB     0C4H,0C4H,0C4H,0C4H,0C4H,0C4H,0C4H,0C4H,0C4H,0C4H,0C4H,0C4H,0C4H
        DB     0C4H,0C4H,0C4H,0C4H,0C4H,0C4H,0C4H,0C4H,0C4H,0C4H,0C4H,0C4H,0C4H
        DB     0C4H,0C4H,0C4H,0C4H,0C4H,0C4H,0C4H,0C4H,0C4H,0C4H,0C4H,0C4H,0C4H
        DB     0C4H,0C4H,0C4H,0C4H,0C4H,0C4H,0C4H,0C4H,0C4H,0C4H,0C4H,0C4H,0C4H
        DB     0C4H,0C4H,0C4H,0C4H,0C4H,0C4H,0C4H,0C4H,0C4H,0C4H,0C4H,0C4H,0C4H
        DB     0BFH,0
        MOV    BX,1*256+0
        CALL   GOTOXY
        CALL   RAWPRT                  ;title (22 + 34 + 22 columns)
        DB     27,'b',7,0B3H,'                      '
        DB     27,'b',14,'FILER CP/M-86 (IBM-PC) version '
        DB     VERS/10+'0','.',(VERS MOD 10)+'0'
        DB     27,'b',7,'                      ',0B3H,0
        MOV    BX,2*256+0
        CALL   GOTOXY
        CALL   RAWPRT                  ;bottom border
        DB     27,'b',7,0D4H
        DB     0CDH,0CDH,0CDH,0CDH,0CDH,0CDH,0CDH,0CDH,0CDH,0CDH,0CDH,0CDH,0CDH
        DB     0CDH,0CDH,0CDH,0CDH,0CDH,0CDH,0CDH,0CDH,0CDH,0CDH,0CDH,0CDH,0CDH
        DB     0CDH,0CDH,0CDH,0CDH,0CDH,0CDH,0CDH,0CDH,0CDH,0CDH,0CDH,0CDH,0CDH
        DB     0CDH,0CDH,0CDH,0CDH,0CDH,0CDH,0CDH,0CDH,0CDH,0CDH,0CDH,0CDH,0CDH
        DB     0CDH,0CDH,0CDH,0CDH,0CDH,0CDH,0CDH,0CDH,0CDH,0CDH,0CDH,0CDH,0CDH
        DB     0CDH,0CDH,0CDH,0CDH,0CDH,0CDH,0CDH,0CDH,0CDH,0CDH,0CDH,0CDH,0CDH
        DB     0BEH,27,'b',15,0
        RET
; inline print of message, no 7-bit strip (box drawing characters)
RAWPRT:
        MOV    BP,SP
        XCHG   BX,[BP]
RAWLP:
        MOV    AL,M
        INC    BX
        OR     AL,AL
        JZ     RAWEND
        CALL   TTYPE
        JMPS   RAWLP
RAWEND:
        MOV    BP,SP
        XCHG   BX,[BP]
        RET
; home the cursor
CUR@FIRST:
        MOV    BX,CURHOME              ; HOME ADDRESS
        MOV    Word Ptr CURAT,BX       ; SET CURSOR POSITION
        JMP    GOTOXY
; last file position
CUR@LAST:
        MOV    BX,Word Ptr RINGPOS     ; ADVANCE
        MOV    Word Ptr LOCPOS,BX      ; SET LOCAL POSITION
CL0:
        MOV    DX,HB_STRIDE
        LAHF
        ADD    BX,DX
        RCR    SI,1
        SAHF
        RCL    SI,1
        XCHG   BX,DX
        MOV    BX,Word Ptr LOCEND      ; END OF LOCAL RING?
        CALL   CMPDEHL
        JNZ    L_C34
        RET
L_C34:
        XCHG   BX,DX                   ; NEW POSITION
        MOV    Word Ptr LOCPOS,BX
        PUSH   BX                      ; SAVE POSITION
        CALL   CUR@NEXT                ; ADVANCE CURSOR
        POP    BX                      ; GET POSITION
        JMPS   CL0
; advance the cursor
CUR@NEXT:
        MOV    BX,Word Ptr CURAT       ; COMPUTE NEW POSITION
        MOV    AL,BL                   ; CHECK FOR NEW LINE
        ADD    AL,19                   ; SIZE OF EACH ENTRY
        CMP    AL,70
        JNB    CN1                     ; ADVANCE TO NEXT LINE
        MOV    BL,AL                   ; NEW POSITION
        MOV    Word Ptr CURAT,BX
        JMP    GOTOXY
CN1:
        MOV    AL,BH                   ; GET LINE
        MOV    BX,CURHOME              ; GET COL
        MOV    BH,AL                   ; SET LINE AND FALL GO TO CUR$DOWN
        MOV    Word Ptr CURAT,BX
        JMPS   CUR@DOWN
; back up the cursor
CUR@BACK:
        MOV    BX,CURHOME              ; GET HOME
        XCHG   BX,DX                   ; ... IN DE
        MOV    BX,Word Ptr CURAT
        CALL   CMPDEHL                 ; COMPARE
        JZ     CUR@LAST                ; GOTO END IF LAST
        MOV    AL,BL                   ; CHECK FOR FIRST COL
        CMP    AL,DL
        JZ     CB1
        SUB    AL,19                   ; BACK UP ONE COL
        MOV    BL,AL
        MOV    Word Ptr CURAT,BX       ; NEW POS
        JMP    GOTOXY
CB1:
        MOV    AL,DL                   ; GET HOME COL
        ADD    AL,19*3                 ; GET LAST COL
        MOV    BL,AL
        DEC    BH                      ; PREV LINE
        MOV    Word Ptr CURAT,BX
        JMP    GOTOXY
; move cursor down one line
CUR@DOWN:
        MOV    BX,CURHOME              ; GET HOME ADDRESS
        MOV    CH,BH                   ; LINE IN B
        MOV    BX,Word Ptr CURAT       ; GET CURRENT ADDRESS
        INC    BH                      ; MOVE DOWN
        MOV    AL,BH                   ; CHECK FOR TOO FAR
        SUB    AL,CH
        CMP    AL,EPS/4
        JNB    CD1
        MOV    Word Ptr CURAT,BX       ; OK, SO SET POSITION
        JMP    GOTOXY
CD1:
        MOV    AL,BL                   ; GET COL
        MOV    BX,CURHOME
        MOV    BL,AL
        MOV    Word Ptr CURAT,BX
        JMP    GOTOXY
; refresh screen
REFRESH:
        MOV    BX,Word Ptr CURAT       ; SAVE CURSOR AND RING POSITIONS
        MOV    Word Ptr SCURAT,BX
        MOV    BX,Word Ptr RINGPOS
        MOV    Word Ptr SRINGPOS,BX
        CALL   CLS                     ; CLEAR SCREEN
        CALL   CUR@OFF                 ; TURN OFF TERM. CURSOR
        CALL   BANNER                  ; PRINT BANNER
        CALL   NEWPOS                  ; DISPLAY FILES
        MOV    BX,CPMADR               ; COMMAND PROMPT MESSAGE
        CALL   GOTOXY
        CALL   ILPRT                   ; PROMPT WITH DRIVE PREFIX
LOG@DU@MSG:
        DB     '   :  '
        DB     27,'b',10,'Command (? = Help)  :',27,'b',15,0
        MOV    BX,SDMADR               ; SCREEN DIRECTORY MESSAGE
        CALL   GOTOXY
        CALL   ILPRT                   ; *commented out -km  
        DB     0                       ; '[ Screen Directory ]',0
        MOV    BX,FNADR                ; PT TO WHERE FILE NAME IS PRINTED
        MOV    BL,1                    ; COL 1 FOR THIS MESSAGE
        CALL   GOTOXY                  ; GO THERE
        CALL   ILPRT
        DB     'Current File:',0
        MOV    BX,Word Ptr SCURAT      ; RESTORE CURSOR AND RING POSITIONS
        MOV    Word Ptr CURAT,BX
        MOV    BX,Word Ptr SRINGPOS
        MOV    Word Ptr RINGPOS,BX
        CALL   SETCUR                  ; RESTORE CURSOR ON SCREEN
        CALL   CUR@ON                  ; TURN ON TERM. CURSOR
        RET

; refresh file display
NEWPOS:
        CALL   CUR@FIRST               ; POSITION CURSOR AT FIRST POSITION
        MOV    BX,Word Ptr LOCBEG      ; PT TO FIRST FILE NAME
        MOV    Word Ptr LOCPOS,BX      ; SAVE LOCAL POSITION
NEWP1:
        MOV    BX,Word Ptr LOCEND      ; AT END?
        XCHG   BX,DX
        MOV    BX,Word Ptr LOCPOS
        CALL   CMPDEHL
        JNZ    L_C35   
        JMP    CUR@FIRST               ; POSITION AT FIRST ENTRY AND RETURN
L_C35:
        MOV    CH,4                    ; 4 SPACE
        MOV    AL,' '
T4:
        CALL   TTYPE
        DEC    CH
        JNZ    T4
        PUSH   BX                      ; SAVE CURRENT LOCAL POSITION IN RING
        LAHF                           ; PT TO FILE NAME
        INC    BX
        SAHF
        CALL   NC@PRFN                 ; PRINT FILE NAME
        POP    BX                      ; GET CURRENT LOCAL POSITION
        MOV    DX,HB_STRIDE
        LAHF
        ADD    BX,DX
        RCR    SI,1
        SAHF
        RCL    SI,1
        MOV    Word Ptr LOCPOS,BX
        CALL   CUR@NEXT                ; ADVANCE CURSOR
        JMPS   NEWP1

; position cursor at CURAT
SETCUR:
        MOV    BX,Word Ptr CURAT

 IF     ENH@VID
        ADD    BX,4                    ;advance 4 spaces
        CALL   GOTOXY
 ENDIF
 IF  NOT ENH@VID
        CALL   GOTOXY
        CALL   ILPRT
        DB     32,32,16,32,0                ;'--> ' *changed -km
 ENDIF
        MOV    BX,Word Ptr RINGPOS     ;pt to current file name
        INC    BX                      ;pt to first char
        CALL   AC@PRFN
        RET

; clear cursor
CLRCUR:
        MOV    BX,Word Ptr CURAT

        IF     ENH@VID
        ADD    BX,4                    ;advance 4 spaces
        CALL   GOTOXY
        ENDIF
        IF  NOT ENH@VID
        CALL   GOTOXY
        CALL   ILPRT
        DB     '    ',0
        ENDIF

        MOV    BX,Word Ptr RINGPOS     ;pt to current file name
        INC    BX                      ;pt to first char
        CALL   NC@PRFN                 ;print file name
        RET

; command prompt
CPRMPT:
        MOV    BX,CPADR                ; GET ADDRESS
MPRINT:
        PUSH   BX                      ; SAVE ADDRESS
        CALL   GOTOXY
        CALL   EREOL                   ; ERASE TO EOL
        POP    BX                      ; GET ADDRESS
        CALL   GOTOXY                  ; POSITION CURSOR
        JMP    ILPRT                   ; PRINT MESSAGE AND RETURN

; working message
WORKMSG:
        CALL   ERMSG
        DB     'Working ...',0
        RET
; error message
ERMSG:
        MOV    AL,0FFH                 ; SET ERROR MESSAGE FLAG
        MOV    Byte Ptr ERMFLG,AL
        MOV    BX,ERADR                ; GET ADDRESS
        JMPS   MPRINT
; print file size info
FSNOTE:
        CALL   ERMSG                   ; USE THIS ROUTINE
        DB     'File Size of ',0
        RET
; position for file size print
ATFS:
        MOV    BX,FSADR+13             ; POSITION FOR PRINT OF FILE SIZE
        JMP    GOTOXY
; clear error message
ERCLR:
        XOR    AL,AL                   ; CLEAR FLAG
        MOV    Byte Ptr ERMFLG,AL
        MOV    BX,ERADR                ; POSITION
        CALL   GOTOXY
        JMP    EREOL                   ; ERASE TO EOL
; position at command prompt and clear it
ATCMD:
        MOV    BX,CPADR                ; POSITION
        CALL   GOTOXY
        CALL   EREOL                   ; CLEAR MESSAGE
        MOV    BX,CPADR                ; REPOSITION
        JMP    GOTOXY
; position at bottom of screen and prompt for continuation

BOTTOM:
        MOV    BX,BOTADR               ; POSITION
        CALL   GOTOXY
        CALL   ILPRT
        DB     CR,LF,'[End of file..]',0
        JMP    KEYIN

ENDHELP:
        MOV    BX,BOTADR1               ; POSITION (bottom-1) 
        CALL   GOTOXY
        CALL   ILPRT
        DB     cr,lf,' Press any key to return. ',0
        JMP    KEYIN

; HIDDEN is an in-memory flag, never a CP/M filename attribute.
; Raw search with user '?' yields CDOS FAT's synthetic header. Version is
; only an initial gate: allocation bytes are decoded ONLY after that marker.
; No BDOS 230, DOS interrupt, sibling library or on-disk change is needed.
HB_PROBE:
        PUSH AX
        PUSH BX
        PUSH CX
        PUSH DX
        MOV CL,SETDMA
        MOV DX,TBUF
        INT 224
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
; Current-file status line has room for distinct S/H flags. Grid width,
; tag offset and filename rendering remain unchanged.
HB_SELECTED_FLAGS:
        PUSH AX
        PUSH BX
        MOV BX,Word Ptr RINGPOS
        MOV AL,Byte Ptr 13[BX]
        PUSH AX
        MOV AL,' '
        CALL TTYPE
        POP AX
        PUSH AX
        TEST AL,4
        MOV AL,' '
        JZ HB_SELECTED_SYS
        MOV AL,'S'
HB_SELECTED_SYS:
        CALL TTYPE
        POP AX
        TEST AL,2
        MOV AL,' '
        JZ HB_SELECTED_HIDDEN
        MOV AL,'H'
HB_SELECTED_HIDDEN:
        CALL TTYPE
        POP BX
        POP AX
        RET
