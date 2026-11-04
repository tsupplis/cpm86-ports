MASS@TU:
        MOV    AL,TRUE                 ;update file totals
        MOV    Byte Ptr FS@FLG,AL      ;of tagged/untagged files
        CALL   CPRMPT
        DB     'Tag/ Untag ALL (from current selection:T/U)? ',0
        CALL   KEYIN                   ;get response
        CMP    AL,'T'
        JZ     MASS@TAG
        CMP    AL,'U'
        JZ     L_A11   
        JMP    NEUTRAL                 ;fall thru to MASS$UNTAG
L_A11:
; mass   u n t a g
MASS@UNTAG:
        XOR    AL,AL                   ;set tag/untag..
        MOV    Byte Ptr T@UN@FG,AL     ;..flag to untag
        MOV    Byte Ptr FSDFLG,AL      ;no file size display
        CALL   WORKMSG
MUTLOOP:
        MOV    BX,Word Ptr RINGPOS     ;move to tag
        MOV    DX,12
        ADD    BX,DX
        MOV    AL,M                    ;get tag
        CMP    AL,'*'                  ;check for tag
        MOV    M,' '                   ;clear tag
        JNZ    L_A12   
        CALL   FSIZ                    ;adjust sizes
L_A12:
        MOV    BX,Word Ptr RINGPOS     ;advance to next
        MOV    DX,HB_STRIDE
        LAHF
        ADD    BX,DX
        SAHF
        MOV    Word Ptr RINGPOS,BX
        XCHG   BX,DX                   ;done?
        MOV    BX,Word Ptr LOCEND
        CALL   CMPDEHL
        JNZ    MUTLOOP
        MOV    BX,CURHOME              ;reset cursor
        MOV    Word Ptr CURAT,BX
        MOV    BX,Word Ptr LOCBEG      ;set ring position
        JMP    JFW0
; mass   t a g
MASS@TAG:
        XOR    AL,AL
        MOV    Byte Ptr FSDFLG,AL      ;no file size display
        MOV    AL,TRUE                 ;set tag/untag..
        MOV    Byte Ptr T@UN@FG,AL     ;..flag to untag
        CALL   WORKMSG
MTLOOP:
        MOV    BX,Word Ptr RINGPOS     ;move to tag
        MOV    DX,12
        ADD    BX,DX
        MOV    AL,M                    ;get tag
        CMP    AL,'*'                  ;check for tag
        MOV    M,'*'                   ;clear tag
        JZ     L_A13   
        CALL   FSIZ                    ;adjust sizes
L_A13:
        MOV    BX,Word Ptr RINGPOS     ;advance to next
        MOV    DX,HB_STRIDE
        LAHF
        ADD    BX,DX
        SAHF
        MOV    Word Ptr RINGPOS,BX
        XCHG   BX,DX                   ;done?
        MOV    BX,Word Ptr LOCEND
        CALL   CMPDEHL
        JNZ    MTLOOP
        MOV    BX,CURHOME              ;reset cursor
        MOV    Word Ptr CURAT,BX
        MOV    BX,Word Ptr LOCBEG      ;set ring position
        JMP    JFW0
; u n t a g
UNTAG:
        XOR    AL,AL                   ;set tag/untag..
        MOV    Byte Ptr T@UN@FG,AL     ;..flag to untag.
        NOT    AL
        MOV    Byte Ptr FS@FLG,AL      ;set flag to compute file size
        MOV    BX,Word Ptr RINGPOS     ;move back one..
        MOV    DX,12                   ;..character position..
        ADD    BX,DX                   ;..and check tagging status.
        MOV    AL,M                    ;if file previously tagged, remove..
        CMP    AL,'*'                  ;..size from..
        LAHF                           ;save flag
        XCHG   AL,AH
        PUSH   AX
        MOV    M,' '                   ; (untag character, to next ring position.)
        CALL   REFFN                   ;refresh file name
        POP    AX                      ;get flag
        XCHG   AL,AH
        SAHF
        JZ     FS2                     ;..summation.
        JMP    FORWARD
; t a g
TAG@EM:
        MOV    AL,TRUE                 ;set..
        MOV    Byte Ptr T@UN@FG,AL     ;..tag/untag and..
        MOV    Byte Ptr FS@FLG,AL      ;..file size flags to tag.
        MOV    BX,Word Ptr RINGPOS
        MOV    DX,12                   ;move back one..
        ADD    BX,DX                   ;..position..
        MOV    AL,M                    ; (if file
        CMP    AL,'*'                  ; already tagged, skip
        JNZ    L_A14   
        JMP    FORWARD                 ; to next file.)
L_A14:
        MOV    M,'*'                   ;..and store a '*' tag character.
        CALL   REFFN                   ;refresh file name
        JMPS   FS2                     ;get file size
; refresh file name with new tag
REFFN:

        MOV    BX,Word Ptr CURAT       ;clear cursor
 IF     ENH@VID
        ADD    BX,4                    ;advance 4 spaces
        CALL   GOTOXY
 ENDIF
 IF  NOT  ENH@VID
        CALL   GOTOXY
        CALL   ILPRT
        DB     '    ',0                ;clear cursor
 ENDIF
        MOV    BX,Word Ptr RINGPOS     ;reprint file name
        INC    BX
        CALL   NC@PRFN                 ;print file name
        RET

; f i l e   s i z e
; determine and display file size in kilobytes -- round up to next disk
; allocation block -- accumulate tagged file summation
FIL@SIZ:
        XOR    AL,AL                   ;set file size/tagged..
        MOV    Byte Ptr FS@FLG,AL      ;..file flag to file size.
        NOT    AL
        MOV    Byte Ptr FSDFLG,AL      ;display file size
        CALL   FSIZ                    ;compute and print file size
        JMP    LOOPFN
FS2:
        MOV    AL,TRUE
        MOV    Byte Ptr FSDFLG,AL      ;display file size
        CALL   FSIZ                    ;compute and print file size
        JMP    FORWARD
;  print file size
FSIZ:
        MOV    AL,Byte Ptr FSDFLG      ;display file size?
        OR     AL,AL                   ;0=no
        JZ     L_A15   
        CALL   FSNOTE
L_A15:
        CALL   RINGFCB                 ;move name to 's$fcb'
; determine file record count and save in 'rcnt'
        MOV    CL,COMPSZ
        MOV    DX,(Offset S@FCB)
        INT    224
        MOV    BX,Word Ptr S@FCB+33
        MOV    Word Ptr RCNT,BX        ;save record count and..
        MOV    BX,0
        MOV    Word Ptr S@FCB+33,BX    ;..reset cp/m.
; round up to next disk allocation block
        MOV    AL,Byte Ptr B@MASK      ;sectors/block - 1
        LAHF                           ;save 'blm'
        XCHG   AL,AH
        PUSH   AX
        XCHG   AL,AH
        MOV    BL,AL
        XCHG   BX,DX
        MOV    BX,Word Ptr RCNT        ;..use here.
        LAHF                           ;round up to next block
        ADD    BX,DX
        RCR    SI,1
        SAHF
        RCL    SI,1
        MOV    CH,3+1                  ;convert from..
        CALL   SHIFTLP                 ;..records to kilobytes.
        POP    AX                      ;retrieve 'blm'
        XCHG   AL,AH
        ROR    AL,1                    ;convert..
        ROR    AL,1                    ;..to..
        ROR    AL,1                    ;..kilobytes/block.
        AND    AL,1FH
        NOT    AL                      ;finish rounding
        AND    AL,BL
        MOV    BL,AL                   ;hl-pair contains # of kilobytes
        MOV    AL,Byte Ptr FS@FLG
        OR     AL,AL
        JZ     D@F@SIZ                 ;branch if 'f' function
; tagged file size summation
        XCHG   BX,DX                   ;file size to de-pair
        MOV    AL,Byte Ptr T@UN@FG
        OR     AL,AL
        JZ     TAKE                    ;if untag, take size from total.
        MOV    BX,Word Ptr TAG@TOT     ;accumulate..
        ADD    BX,DX                   ;..sum of..
        MOV    Word Ptr TAG@TOT,BX     ;..tagged file sizes.
        XCHG   BX,DX                   ;file size to hl-pair
        JMPS   D@F@SIZ                 ;branch to display sizes
TAKE:
        MOV    BX,Word Ptr TAG@TOT     ;subtract..
        MOV    AL,BL                   ;..file..
        SUB    AL,DL                   ;..size..
        MOV    BL,AL                   ;..from..
        MOV    AL,BH                   ;..summation..
        SBB    AL,DH                   ;..total.
        MOV    BH,AL                   ;then put..
        MOV    Word Ptr TAG@TOT,BX     ; (save total)
        XCHG   BX,DX                   ;..file size in hl-pair.
; display file size in kilobytes -- right justify tagged file total
D@F@SIZ:
        MOV    AL,Byte Ptr FSDFLG      ;display file size?
        OR     AL,AL                   ;0=no
        JNZ    L_A16
        RET
L_A16:
        PUSH   BX                      ;save value
        CALL   ATFS                    ;position for file size print
        MOV    BX,Word Ptr RINGPOS     ;print file name of current file
        LAHF
        INC    BX
        SAHF
        CALL   PRFN
        MOV    AL,':'
        CALL   TTYPE
        POP    BX                      ;get value
        CALL   DECOUT                  ;print individual file size
        MOV    AL,'K'
        CALL   TTYPE
; determine # of digits in tagged summation
        MOV    BX,Word Ptr TAG@TOT     ;get present summation
        CALL   ILPRT
        DB     ' Tagged:',0
        CALL   DECOUT                  ;print tagged file summation
        MOV    AL,'K'
        JMP    TTYPE
; j u m p
; backward
JUMPB:
        CALL   CUR@OFF                 ;turn off term. cursor
        MOV    BX,CURHOME              ;set cursor home
        MOV    Word Ptr CURAT,BX
        MOV    BX,Word Ptr RING        ;at front?
        XCHG   BX,DX
        MOV    BX,Word Ptr LOCBEG
        CALL   CMPDEHL
        JZ     JUMPBW                  ;back up and wrap around
        MOV    Word Ptr LOCEND,BX      ;set new end
        MOV    DX,-EPS*HB_STRIDE              ;back up
        LAHF
        ADD    BX,DX
        RCR    SI,1
        SAHF
        RCL    SI,1
        MOV    Word Ptr LOCBEG,BX      ;new beginning
        MOV    Word Ptr RINGPOS,BX     ;new position
        CALL   REFRESH                 ;refresh screen
        CALL   CUR@ON                  ;turn on term. cursor
        JMP    LOOPFN
JUMPBW:
        MOV    BX,Word Ptr LOCBEG      ;at first screen?
        XCHG   BX,DX
        MOV    BX,Word Ptr RING        ;pt to first element of ring
        CALL   CMPDEHL
        JZ     JBW0                    ;advance to end
        MOV    BX,-EPS*HB_STRIDE              ;back up
        LAHF
        ADD    BX,DX
        SAHF
        JMPS   JFW0
JBW0:
        MOV    DX,EPS*HB_STRIDE               ;pt to next screen
        LAHF
        ADD    BX,DX
        RCR    SI,1
        SAHF
        RCL    SI,1
        XCHG   BX,DX
        MOV    BX,Word Ptr RINGEND
        CALL   CMPDEHL
        XCHG   BX,DX
        JZ     JBW1
        JB     JBW0
JBW1:
        MOV    DX,-EPS*HB_STRIDE
        LAHF                           ;pt to first element of local ring
        ADD    BX,DX
        SAHF
        JMPS   JFW0
JUMPF:
        CALL   CUR@OFF                 ;tuen off term. cursor
        MOV    BX,CURHOME              ;set cursor to home
        MOV    Word Ptr CURAT,BX
        MOV    BX,Word Ptr LOCEND      ;see if Local End <= Ring End
        XCHG   BX,DX
        MOV    BX,Word Ptr RINGEND
        CALL   CMPDEHL
        JZ     CMDLOOP
        MOV    BX,Word Ptr LOCEND      ;new screen
        JMPS   JFW0
CMDLOOP:
        CALL   CUR@OFF                 ;turn off cursor
        MOV    BX,CURHOME              ;set cursor home
        MOV    Word Ptr CURAT,BX
        MOV    BX,Word Ptr RING        ;set ring position
JFW0:
        MOV    Word Ptr RINGPOS,BX
JFW0A:
        MOV    Word Ptr LOCBEG,BX      ;front of ring
        MOV    DX,EPS*HB_STRIDE               ;new end?
        LAHF
        ADD    BX,DX
        RCR    SI,1
        SAHF
        RCL    SI,1
        XCHG   BX,DX
        MOV    BX,Word Ptr RINGEND     ;end of ring
        XCHG   BX,DX
        CALL   CMPDEHL
        JB     JFW1
        XCHG   BX,DX
JFW1:
        XCHG   BX,DX
        MOV    Word Ptr LOCEND,BX
        CALL   REFRESH
        CALL   CUR@ON                  ;turn on term. cursor
        JMP    LOOPFN

; f o r w a r d
FORWARD:
        CALL   CUR@OFF                 ;turn off term. cursor
        CALL   CLRCUR                  ;clear cursor
        CALL   FOR0                    ;position on screen and in ring
        CALL   SETCUR                  ;set cursor
        CALL   CUR@ON                  ;turn on term. cursor
        JMP    LOOPFN
;  advance routine
FOR0:
        MOV    BX,Word Ptr RINGPOS     ;at end of loop yet?
        MOV    DX,HB_STRIDE                   ;i.e., will we be at end of loop?
        LAHF
        ADD    BX,DX
        RCR    SI,1
        SAHF
        RCL    SI,1
        XCHG   BX,DX
        MOV    BX,Word Ptr LOCEND
        CALL   CMPDEHL                 ;compare 'present' to 'end'
        JNZ    FORW                    ;to next print position
        CALL   CUR@FIRST               ;position cursor
        MOV    BX,Word Ptr LOCBEG      ;set position pointer to beginning and..
        MOV    Word Ptr RINGPOS,BX
        RET
FORW:
        MOV    BX,Word Ptr RINGPOS     ;advance in ring
        MOV    DX,HB_STRIDE
        LAHF
        ADD    BX,DX
        RCR    SI,1
        SAHF
        RCL    SI,1
        MOV    Word Ptr RINGPOS,BX     ;new position
        CALL   CUR@NEXT                ;position cursor
        RET
; r e v e r s e
REVERSE:
        CALL   CUR@OFF                 ;turn off term. cursor
        CALL   CLRCUR                  ;clear cursor
        CALL   REV0                    ;position on screen and in ring
        CALL   SETCUR                  ;set cursor
        CALL   CUR@ON                  ;turn on term. cursor
        JMP    LOOPFN
;  Back Up Routine
REV0:
        MOV    BX,Word Ptr LOCBEG
        XCHG   BX,DX
        MOV    BX,Word Ptr RINGPOS     ;see if at beginning of ring
        CALL   CMPDEHL
        JNZ    REV1                    ;skip position pointer reset if not..
        CALL   CUR@LAST                ;end of local ring
        MOV    BX,Word Ptr LOCEND      ;set to end +1 to backup to end
        MOV    DX,-HB_STRIDE
        LAHF
        ADD    BX,DX
        RCR    SI,1
        SAHF
        RCL    SI,1
        MOV    Word Ptr RINGPOS,BX
        RET
REV1:
        CALL   CUR@BACK                ;back up 1
REV2:
        MOV    BX,Word Ptr RINGPOS
        MOV    DX,-HB_STRIDE                  ;one ring position..
        LAHF                           ;..backwards.
        ADD    BX,DX
        RCR    SI,1
        SAHF
        RCL    SI,1
        MOV    Word Ptr RINGPOS,BX
        RET
; u p
UP:
        CALL   CUR@OFF                 ;turn off term. cursor
        CALL   CLRCUR                  ;clear cursor
        MOV    BX,Word Ptr RINGPOS     ;see if wrap around
        MOV    DX,-HB_STRIDE*4                ;4 entries
        LAHF
        ADD    BX,DX
        RCR    SI,1
        SAHF
        RCL    SI,1
        XCHG   BX,DX
        MOV    BX,Word Ptr LOCBEG      ;beginning of local screen
        CALL   CMPDEHL
        JB     UP2                     ;wrap around
        MOV    CH,4                    ;back up 4 entries
UP1:
        PUSH   CX                      ;save count
        CALL   REV0                    ;back up in ring and on screen (no print)
        POP    CX                      ;get count
        DEC    CH
        JNZ    UP1
        JMPS   DOWN1A
UP2:
        MOV    BX,Word Ptr RINGPOS     ;advance to beyond end
        MOV    DX,HB_STRIDE*4
        LAHF
        ADD    BX,DX
        RCR    SI,1
        SAHF
        RCL    SI,1
        XCHG   BX,DX
        MOV    BX,Word Ptr LOCEND      ;compare to local end
        XCHG   BX,DX
        CALL   CMPDEHL
        JZ     DOWN1A                  ;at end, so too far
        JB     DOWN1A                  ;beyond end, so back up
        MOV    Word Ptr RINGPOS,BX     ;new ring position
        MOV    BX,Word Ptr CURAT       ;advance cursor
        INC    BH                      ;next line
        MOV    Word Ptr CURAT,BX
        JMPS   UP2
; d o w n
DOWN:
        CALL   CUR@OFF                 ;turn off term. cursor
        CALL   CLRCUR                  ;clear cursor
        MOV    BX,Word Ptr RINGPOS     ;see if wrap around
        MOV    DX,HB_STRIDE*4                 ;4 entries
        LAHF
        ADD    BX,DX
        RCR    SI,1
        SAHF
        RCL    SI,1
        XCHG   BX,DX
        MOV    BX,Word Ptr LOCEND      ;end of local screen
        XCHG   BX,DX
        CALL   CMPDEHL
        JZ     DOWN2                   ;wrap around
        JB     DOWN2                   ;wrap around
        MOV    CH,4                    ;forward 4 entries
DOWN1:
        PUSH   CX                      ;save count
        CALL   FOR0                    ;advance in ring and on screen (no print)
        POP    CX                      ;get count
        DEC    CH
        JNZ    DOWN1
DOWN1A:
        CALL   SETCUR                  ;set cursor
        CALL   CUR@ON                  ;turn on term. cursor
        JMP    LOOPFN
DOWN2:
        MOV    BX,Word Ptr CURAT       ;preserve column
        MOV    CH,BL                   ;column number in B
        MOV    BX,CURHOME              ;home position
        MOV    Word Ptr CURAT,BX       ;set new position
        MOV    BX,Word Ptr LOCBEG      ;beginning of local ring
        MOV    Word Ptr RINGPOS,BX     ;new ring position
DOWN3:
        MOV    BX,Word Ptr CURAT       ;check for at top of column
        MOV    AL,BL                   ;get col
        CMP    AL,CH                   ;there?
        JZ     DOWN1A
        MOV    BX,Word Ptr RINGPOS     ;advance in ring
        MOV    DX,HB_STRIDE                   ;13 bytes/entry
        ADD    BX,DX
        MOV    Word Ptr RINGPOS,BX
        MOV    BX,Word Ptr CURAT       ;get cursor position
        MOV    DX,19                   ;advance 19 bytes/screen entry
        ADD    BX,DX
        MOV    Word Ptr CURAT,BX
        JMPS   DOWN3
; s t a t
; determine remaining storage on requested disk
R@DR@ST:
        CALL   CPRMPT
        DB     'Status of Disk: ',0
        CALL   KEYIN                   ;get char
        LAHF
        XCHG   AL,AH
        PUSH   AX
        CALL   CRLF
        POP    AX
        XCHG   AL,AH
        SUB    AL,'A'                  ;convert to number
        JNB    L_A17   
        JMP    NEUTRAL
L_A17:
        MOV    CH,AL                   ;... in B
        MOV    AL,Byte Ptr MAXDR       ;compare to max
        SUB    AL,'A'
        CMP    AL,CH
        JNB    L_A18   
        MOV    AL,Byte Ptr MDRIVE
        DEC    AL
        CMP    AL,CH
        JE     L_A18
        JMP    LOOPFN
L_A18:
        MOV    AL,CH                   ;get disk
        MOV    Byte Ptr R@DR,AL        ;requested drive
        CALL   RESET                   ;..login as current.
        CALL   FRESTOR                 ;determine free space remaining
        CALL   PRINT@FRE               ;print value
        MOV    AL,Byte Ptr C@DR        ;login original as..
        CALL   SET@DR                  ;..current drive.
        JMP    LOOPFN                  ;done


; d e l e t e

; mass delete
