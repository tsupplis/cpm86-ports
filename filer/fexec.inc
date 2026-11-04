HELP:
        CALL   HELPMSG                 ;print message
        CALL   REFRESH                 ;refresh screen
        JMP    LOOPFN
HELPMSG:
        CALL   CLS
        CALL   ILPRT
        DB     27,'b',14
        DB     ' FILER CP/M-86 (IBM-PC)           COMMAND HELP                 Version '
        DB     VERS/10+'0','.',(VERS MOD 10)+'0',27,'b',15,cr,lf
        DB     cr,lf
        DB     27,'b',10,cr,lf
        DB     ' [ Tagging Commands ]         [ File Operations ]         [ File Print & View ]',cr,lf
        DB     27,'b',15,'   T - Tag File                 C - Copy File               ^P - Print',cr,lf
        DB     '   U - Untag File               D - Delete File              V - View',cr,lf
        DB     '   W - Mass Tag/Untag           F - File Size                   <SP> Scroll one',cr,lf
        DB     '   M - Mass Copy                R - Rename File                 line forward',cr,lf
        DB     '   Y - Mass Delete',cr,lf
        DB     '   I - Tag Info                O - Toggle normal/all (SYSTEM/HIDDEN)',cr,lf
        DB '   Selected-file status: S=SYSTEM, H=FAT HIDDEN',cr,lf
        DB     27,'b',10,cr,lf
        DB     ' [ Directory Movement ]       [ CP/M-86 System Related ]',cr,lf
 IF     HLP@FIL
        DB     ' <SP> Scroll Line Forward       H - Detailed HELP',CR,LF
 ENDIF
        DB     27,'b',15,'  < > - Entry Back,Forth      ?,/ - This Help Screen',cr,lf
        DB     0
        CMP    Byte Ptr CCKEYS,0
        JE     HELPECHAIN
        CALL   ILPRT
        DB 27,'b',15,'   ',16,'  - Entry Right             E - Enter command line',cr,lf,0
        JMP    HELPEDONE
HELPECHAIN:
        CALL   ILPRT
        DB 27,'b',15,'   ',16,'  - Entry Right             E - Enter command line[*]',cr,lf
        DB     0
HELPEDONE:
        CALL   ILPRT
        DB 27,'b',15,'   ',17,'  - Entry Left              L - Login DIR[USR#]   c0 = drv[c][usr0]',cr,lf
        DB '   ',30,'  - Entry Up                N - reNew current Screen',cr,lf
        DB '   ',31,'  - Entry Dn                S - Status of disk [drv]',cr,lf
        DB      ' PgUp - Next Page         [Esc],Q - Quit FILER',cr,lf
        DB      0
        CMP    Byte Ptr CCKEYS,0
        JNE    HELPCLI
        JMP    HELPCHAIN
HELPCLI:
        CALL   ILPRT
        DB     27,'b',15,' PgUp - Prev Page               X - eXecute CMD file',cr,lf
        DB     27,'b',10,cr,lf
        DB     '                               E/X return to FILER after the command ends.',cr,lf
        DB     0
        JMP    HELPEXECEND
HELPCHAIN:
        CALL   ILPRT
        DB      27,'b',15,' PgUp - Prev Page               X - eXecute CP/M-86 CMD file[*]',cr,lf
        DB      27,'b',10,cr,lf
        DB      '                               [*]Does NOT return to FILER',cr,lf
        DB     0
HELPEXECEND:
        CALL   ILPRT
        DB     '                               F1-F9 keys not yet implemented.',cr,lf
        DB     cr,lf,0
        CALL   ENDHELP
        RET

EXTHELP:
        TEST   HELPFLG,TRUE
        JZ     EHLP1                   ;jmp if further help not avail
        CALL   CLS
        CALL   ILPRT
        DB     'Running External HELP ....',0
        MOV    SI,OFFSET MOREHELP      ;run HELP command
        MOV    DI,TBUF
        MOV    CX,13
        CALL   CHAIN
        JMP    LOOPFN

EHLP1:
        CALL   ERMSG
        DB     'External HELP Facility Not Available',0
        JMP    LOOPFN

; refresh screen
SCREFRESH:
        CALL   REFRESH                 ;do it
        JMP    LOOPFN                  ;reprint name

CHK@CMD:                               ;check if CMD file
;      ENTRY:  file in RINGPOS
;      EXIT:   zero set if CMD type
;
        PUSH   BX
        MOV    BX,Word Ptr RINGPOS     ;get addr file location
        ADD    BX,9                    ;point to file type
        CMP    Byte Ptr[BX],'C'        ;test for CMD
        JNE    CC_RET
        INC    BX
        CMP    Byte Ptr[BX],'M'
        JNE    CC_RET
        INC    BX
        CMP    Byte Ptr[BX],'D'
CC_RET: POP     BX
        RET                            ;zero set if CMD type

XQT@CMD:
        CALL   CPRMPT
        DB     'Execute Current CMD File (Y/N)? ',0
        CALL   KEYIN
        CMP    AL,'Y'
        JE     XQT1
        JMP    NEUTRAL
XQT1:
        CALL   CHK@CMD                 ;check if CMD filetype
        JZ     XQT2                    ;give error if not
        CALL   ERMSG
        DB     'Only CMD type files can be executed',0
        JMP    LOOPFN

XQT2:  CALL    CLS
        MOV    DI,TBUF                 ;put CMD in default DMA buffer
        MOV    SI,Word Ptr RINGPOS     ;get position in ring
        MOV    AL,[SI]                 ;get drive number
        ADD    AL,'A'-1                ;make into letter
        MOV    Byte Ptr[DI],AL         ;move it
        INC    SI ! INC DI             ;SI -> filename
        MOV    Byte Ptr[DI],':'        
        INC    DI                      ;DI -> TBUF+2
        MOV    CX,8                    ;bytes in filename
        JMP    CHAIN

RUN@CMD:
        CALL   ERMSG
        DB     'Enter Command? ',0
        MOV    BX,OFFSET CRCTBL+256    ;use last half of CRC table
        MOV    M,102                   ;allow 100 length of line
        INC    BX
        MOV    M,0                     ;store count
        MOV    DX,OFFSET CRCTBL+256
        PUSH   DX
        MOV    CL,RDBUF
        INT    224
        POP    BX                      ;BX -> beginning of buffer
        INC    BX                      ;point to char count
        MOV    CL,M                    ;put in CL
        INC    BX                      ;point to first char
        MOV    CH,0                    ;CX = char count
        PUSH   BX                      ;save current ptr for later
        ADD    BX,CX
        MOV    M,0                     ;store ending zero
        POP    BX                      ;point to first char
        CMP    Byte Ptr[BX],0          ;abort if no first char
        JZ     NO_CMD
        MOV    SI,BX                   ;SI -> Command
        MOV    DI,TBUF                 
        CALL   UC_CMD                  ;make sure its upper CASE
        CALL   CLS
        JMP    RUN_CMD1
NO_CMD: JMP     NEUTRAL                 

RUN_CMD1:
        CALL   CHAIN
        JMP    LOOPFN

CHAIN:
;      ENTRY:  SI -> buffer containing command
;              DI -> dest of command
;              CX =  length of command

        MOV    AX,Word Ptr DATA_SEG
        MOV    ES,AX

        CLD                            ;set forward direction
        REP    MOVSB                   ;move it
        MOV    ES:Byte Ptr [DI],0      ;end with a zero

        CALL   KEYOFF                  ;normal keys for the action
        CMP    Byte Ptr CCKEYS,0
        JNE    RUNCLI
        MOV    CL,47                   ;CP/M-86 retains P_CHAIN
        MOV    DX,0FFFFH
        INT    224
        JMP    RUNFINISH

; Resident concurrent execution, using the native P_CLI control block.
; Priority is set before launch, ahead of both the parent and TMP.
RUNCLI:
        MOV    Byte Ptr CLIERR,0
        MOV    CX,32
        MOV    DX,0FFH
        INT    224
        MOV    CLIUSER,AL
        MOV    CX,25
        INT    224
        MOV    CLIDRIVE,AL
        MOV    CX,156                  ;P_PDADR: ES:BX -> our descriptor
        INT    224
        MOV    AL,ES:Byte Ptr 5[BX]
        MOV    CLIPRIO,AL
        MOV    BX,ES:Word Ptr 1EH[BX]
        MOV    DX,197                  ;no parent: native SUBMIT fallback
        OR     BX,BX
        JZ     CLIPSET
        XOR    DX,DX
        MOV    DL,ES:Byte Ptr 5[BX]
        CMP    DX,64
        JA     CLIPSAFE
        JMP    CLINOSAFE
CLIPSAFE:
        DEC    DX
        CMP    DX,197
        JBE    CLIPSET
        MOV    DX,197
CLIPSET:
        MOV    CX,145                  ;P_PRIORITY
        INT    224
        MOV    AX,Word Ptr DATA_SEG
        MOV    ES,AX
        MOV    SI,TBUF
        MOV    DI,OFFSET CLICB+1
        MOV    CX,128
        CLD
        REP    MOVSB                   ;bounded, NUL-terminated by CHAIN
        MOV    Byte Ptr CLICB,0
        MOV    Byte Ptr CLICB+129,0
        MOV    DX,OFFSET CLICB
        MOV    CX,150                  ;P_CLI
        INT    224
        OR     BX,BX                   ;native CLI returns launch status in BX
        JZ     CLIWAIT
        MOV    Byte Ptr CLIERR,1
        JMP    CLIPRESTORE
CLIWAIT:
        XOR    DX,DX
        MOV    CX,146                  ;C_ATTACH: wait before screen I/O
        INT    224
CLIPRESTORE:
        XOR    DX,DX
        MOV    DL,CLIPRIO
        MOV    CX,145
        INT    224
        JMP    CLICONTEXT
CLINOSAFE:
        MOV    Byte Ptr CLIERR,1        ;do not launch at a system priority
CLICONTEXT:
        XOR    DX,DX
        MOV    DL,CLIUSER
        MOV    CX,32
        INT    224
        XOR    DX,DX
        MOV    DL,CLIDRIVE
        MOV    CX,14
        INT    224
        MOV    DX,TBUF
        MOV    CX,26                   ;restore default DMA
        INT    224
RUNFINISH:
        MOV    AX,Word Ptr DATA_SEG
        MOV    ES,AX
        CALL   KEYON
        MOV    SP,OFFSET STACK         ;CHAIN callers use both CALL and JMP
        CALL   REFRESH
        CMP    Byte Ptr CCKEYS,0
        JE     RUNREADY
        CMP    Byte Ptr CLIERR,0
        JE     RUNREADY
        CALL   ERMSG
        DB     'Cannot launch command',0
RUNREADY:
        JMP    LOOPFN

UC_CMD:        ;make string ending in 0h upper case
;      ENTRY:  BX -> string
;

UC_LOOP:
        CMP    Byte Ptr[BX],0
        JZ     UC_RET                  ;quit if zero
        CMP    Byte Ptr[BX],61h
        JB     UC_OK                   ;skip unless lower case
        AND    Byte Ptr[BX],5FH        ;make uppercase
UC_OK: INC     BX                      ;point to next char
        JMP    UC_LOOP                 ;and process it
UC_RET:
        RET

; mass tag or untag
