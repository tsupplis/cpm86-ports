FILER:
        MOV    AX,DS
        CLI
        MOV    SS,AX
        MOV    SP,(Offset STACK)       ;start local stack
        STI
        MOV    Word Ptr DATA_SEG,AX    ;save data segment
        CALL   KEYINIT                 ;concurrent raw function keys
        CALL   IDU                     ;set initial disk/user
        MOV    BX,(Offset BUFENTRY)    ;base address
        MOV    Word Ptr RING,BX        ;beginning of ring
        MOV    AL,Byte Ptr MDISK       ;get max disk number
        ADD    AL,'A'-1
        MOV    Byte Ptr MAXDR,AL       ;set letter
        MOV    AL,Byte Ptr .FCB+1      ;check for initial help
        CMP    AL,'/'
        JE     FILERH
        MOV    AL,Byte Ptr .FCB2+1     ;check for wait
        CMP    AL,'W'
        JNE    FILER0

FILERSAK:
        CALL   ILPRT
        DB     CR,LF,'Strike Any Key to Enter FILER -- ',0
        CALL   DKEYIN
        JMPS   FILER0
FILERH:
        CALL   HELPMSG                 ;print help message
FILER0:
        IF     HLP@FIL
        CALL   HELPCHK                 ;check for availability of HELP Files
        ENDIF
        JMPS   EMBARK
;  set initial disk/user
IDU:
        MOV    BX,FCB+1                ;check for DU specification
        MOV    AL,M
        CMP    AL,'/'                  ;is there a help request?
        JZ     IDU1
        CMP    AL,' '                  ;<SP>=none
        JZ     IDU1
        CALL   DEF@DU0                 ;extrace drive/user
        MOV    AL,CL                   ;get current user
        CALL   IDU@USET                ;set it
        MOV    AL,CH                   ;get current disk
        CALL   IDU@DSET                ;set it
        JMP    LOG1Z                   ;log it in
IDU1:
        MOV    DL,GET                  ;determine..
        CALL   GET@USR                 ;..user area then..
        CALL   IDU@USET                ;set current user
        MOV    CL,INQDISK              ;determine current disk
        INT    224
        CALL   IDU@DSET                ;set current disk
        JMP    LOG1Z                   ;set current user and disk
IDU@USET:
        MOV    Byte Ptr C@U@A,AL       ;..store as current and..
        MOV    Byte Ptr O@USR,AL       ;..as original for exit.
        MOV    Byte Ptr R@U@A,AL       ;..requested user area
        RET
IDU@DSET:
        MOV    Byte Ptr C@DR,AL
        MOV    Byte Ptr R@DR,AL        ;requested disk
        RET
; check for availability of HELP Files (HELP.CMD)
HELPCHK:
        XOR    AL,AL                   ;assume NO
        MOV    Byte Ptr HELPFLG,AL     ;set flag
        MOV    DX,(Offset HELPFCB)
        CALL   FILECHK                 ;check for file
        OR     AL,AL                   ;0=no
        JNZ    L_A3
        RET
L_A3:
        MOV    AL,0FFH                 ;set flag
        MOV    Byte Ptr HELPFLG,AL
        MOV    AL,Byte Ptr Z@DR        ;Get disk from FFIND
        ADD    AL,'A'                  ;convert to ASCII
        MOV    Byte Ptr MOREHELP,AL    ;put it in filename
        RET

; check for existance of file whose first 12 FCB bytes are pted to by DE
;   return with A=0 if not found, A=0FFH if found
FILECHK:
        MOV    BX,(Offset S@FCB)       ;copy into FCB
        XCHG   BX,DX
        MOV    CH,12                   ;12 bytes
        CALL   MOVE                    ;copied into S$FCB
        XCHG   BX,DX                   ;HL pts to FCB
        CALL   INITFCB                 ;init FCB
        MOV    DX,(Offset S@FCB)       ;pt to FCB
        JMP    FFIND
; determine if specific file(s) requested -- show remaining storage
EMBARK:
        CALL   FRESTOR                 ;get bytes remaining on drive (decode default)
        MOV    BX,(Offset JOKER)       ;..treat as '*.*' with 'joker'..
        MOV    DX,FCB+1                ;..loaded here.
        MOV    CH,11                   ; # of characters to move
        CALL   MOVE                    ;set field to *.*
; build 'ring' with filename positioned in default 'fcb' area
PLUNGE:
        CALL HB_PROBE
        MOV    CL,SETDMA               ;initialize dma address..
        MOV    DX,TBUF                 ;..to default buffer.
        INT    224
        XOR    AL,AL                   ;clear search 'fcb'..
        MOV    Byte Ptr .FCBEXT,AL     ;extent byte..
        MOV    Byte Ptr .FCBRNO,AL     ;..and record number.
        NOT    AL
        MOV    Byte Ptr CANFLG,AL      ;make cancel flag true
        MOV    DX,FCB                  ;default 'fcb' for search..
        MOV    CL,SRCHF                ;..of first occurrence.
        INT    224
        INC    AL                      ; 0ffh --> 00h if no file found
        JZ     L_A4    
        JMP    SETRING                 ;if found, branch and build ring.
L_A4:
        MOV    Byte Ptr CANFLG,AL      ;make log-cancel toggle false
        CALL   ERMSG                   ;else say none found, fall thru to log.
        DB     'File not found',0
; l o g
; select drive and user area (system reset for disk change on-the-fly)
LOG:
        CALL   CPRMPT                  ;prompt to get drive/user selection
        DB     'Login DIR[u]: ',0
        CALL   DEF@D@U
LOG1:
        CALL   LOG1Z                   ;set current and log in
        CALL   CRLF                    ;new line
        JMPS   EMBARK                  ;..restart
;  set current user and disk
LOG1X:
        MOV    BX,(Offset LOG@DU@MSG)
        MOV    AL,Byte Ptr R@DR        ;set prompt message
        ADD    AL,'A'                  ;adjust to letter
        MOV    M,AL
        INC    BX
        MOV    M,' '                   ;prep for user < 10
        MOV    AL,Byte Ptr R@U@A       ;get user
        CMP    AL,10                   ;less than 10?
        JB     LOG2
        MOV    CH,'1'                  ;set digits
LOG1A:
        SUB    AL,10                   ;adjust user
        CMP    AL,10                   ;less?
        JB     LOG1B
        INC    CH                      ;incr 10's
        JMPS   LOG1A
LOG1B:
        MOV    M,CH                    ;set 10's
LOG2:
        INC    BX                      ;pt to 1's
        ADD    AL,'0'                  ;to ASCII
        MOV    M,AL
        RET
;  actually log into DU requested
LOG1Y:
        MOV    AL,Byte Ptr R@U@A       ;establish requested area..
        MOV    Byte Ptr C@U@A,AL       ;..as current area.
        CALL   SET@USR
        CALL   RESET                   ;reset disk system, make requested current.
        MOV    BX,0                    ;initialize tagged..
        MOV    Word Ptr TAG@TOT,BX     ;..file size accumulator.
        RET
;  set current DU and log into it
LOG1Z:
        CALL   LOG1X                   ;set current
        CALL   LOG1Y                   ;actually log in
        RET
; routine to define current drive and user area with full error trapping.
; (check validity of user area entry first, then drive validity, then proceed
; with implementation.)
DEF@D@U:
        MOV    BX,(Offset CMDBUF)+2
        MOV    CH,20                   ; # of blanks to..
        CALL   FILL                    ;..clear 'cmdbuf'.
        MOV    DX,(Offset CMDBUF)      ;get DU selection from..
        MOV    CL,RDBUF                ;..console buffer read.
        INT    224
        CALL   CONVERT                 ;make sure alpha is upper case
        MOV    BX,(Offset CMDBUF)+2    ;pt to possible drive
DEF@DU0:
        CALL   ZDNFIND         ;look for DU form and return DU
        JZ     ERRET                   ;error
        MOV    AL,CH                   ;return disk and user
        MOV    Byte Ptr R@DR,AL
        INC    AL                      ;set FCB
        MOV    Byte Ptr .FCB,AL
        MOV    AL,CL
        MOV    Byte Ptr R@U@A,AL
        RET
; error return and recovery from command cancellation
ERRET:
        CALL   ERMSG
        DB     'DIR Entry Error',0
        JMP    NEUTRAL
COMCAN:
        MOV    SP,(Offset STACK)       ;reset stack..
        MOV    AL,Byte Ptr CANFLG
        OR     AL,AL                   ;..from..
        JNZ    L_A5    
        JMP    PLUNGE
L_A5:
        CALL   REFRESH                 ;refresh screen
        JMP    LOOPFN                  ;..error/command abort.
; find file along path (file FCB pted to by DE)
;   on return, A=0FFH if found, A=0 if not found, and flags set
FFIND:
        PUSH   DX                      ;save ptr to FCB
        MOV    DL,GET                  ;get and save current DU
        CALL   GET@USR
        MOV    Byte Ptr C@U@A,AL
        MOV    Byte Ptr Z@U@A,AL
        MOV    CL,INQDISK
        INT    224
        MOV    Byte Ptr C@DR,AL
        MOV    Byte Ptr Z@DR,AL
        POP    DX                      ;get ptr to FCB
        MOV    BX,(Offset INTPATH)     ;internal path
FFINDL:
        CALL   SEARF                   ;look for file
        JNZ    FFOUND                  ;found file
        MOV    AL,Byte Ptr CINDIC      ;get current indictor
        MOV    CL,AL                   ;... in C
        MOV    AL,M                    ;get drive
        OR     AL,AL                   ;0=done=not found
        JZ     FNFOUND
        CMP    AL,CL                   ;current disk?
        JNZ    FF1
        MOV    AL,Byte Ptr C@DR        ;get current disk
        INC    AL                      ;increment for following DCR
FF1:
        DEC    AL                      ;adjust to 0 for A
        MOV    CH,AL                   ;disk in B
        MOV    Byte Ptr Z@DR,AL        ;note disk
        INC    BX                      ;pt to user
        MOV    AL,M                    ;user in A
        CMP    AL,CL                   ;current?
        JNZ    FF2
        MOV    AL,Byte Ptr C@U@A       ;get current user
FF2:
        MOV    CL,AL                   ;user in C
        MOV    Byte Ptr Z@U@A,AL       ;note user
        LAHF                           ;pt to next entry
        INC    BX
        SAHF
        CALL   SLOGIN                  ;log in DU
        JMPS   FFINDL
FFOUND:
        CALL   DLOGIN                  ;log in default
        MOV    AL,0FFH                 ;set flag
        OR     AL,AL
        RET
FNFOUND:
        CALL   DLOGIN                  ;log in default
        XOR    AL,AL                   ;set flag
        RET
; search for file pted to by DE; don't affect DE or HL; ret code in A
SEARF:
        PUSH   BX                      ;save regs
        PUSH   DX
        MOV    CL,SRCHF                ;search for file
        INT    224
        INC    AL                      ;set flags
        POP    DX                      ;get regs
        POP    BX
        RET
; log in default directory
DLOGIN:
        MOV    AL,Byte Ptr C@DR        ;disk in B
        MOV    CH,AL
        MOV    AL,Byte Ptr C@U@A       ;user in C
        MOV    CL,AL                   ;fall thru to SLOGIN
; log in DU in BC
SLOGIN:
        PUSH   BX                      ;save regs
        PUSH   DX
        PUSH   CX
        MOV    AL,CL                   ;set user
        CALL   SET@USR
        POP    CX
        MOV    AL,CH                   ;set disk
        CALL   SET@DR
        POP    DX                      ;restore regs
        POP    BX
        RET

QUIT:
        CALL   CPRMPT
        DB     'Quit (Y/N)? ',0
        CALL   KEYIN
        CMP    AL,'Y'
        JE     CPM@CCP
        JMP    NEUTRAL

; e x i t
; return to cp/m ccp
CPM@CCP:
        CALL   KEYOFF                  ;normal expansion before returning
        MOV    AL,Byte Ptr O@USR       ;get and set original..
        CALL   SET@USR                 ;..user area and..
        MOV    DX,TBUF                 ;..tidy up..
        MOV    CL,SETDMA               ;..before going home.
        INT    224
        CALL   CLS
        MOV    CL,0
        MOV    DL,0
        INT    224
;warmboot
; establish ring (circular list) of filenames
SETRING:
        MOV    BX,Word Ptr RING        ;initialize ring pointer
        MOV    Word Ptr RINGPOS,BX     ;start --> current position of ring
; put each found name in ring.  a-reg --> offset into 'tbuf' name storage
TO@RING:
        DEC    AL                      ;un-do 'inr' from above and below
        ADD    AL,AL                   ;times 32 --> position index
        ADD    AL,AL
        ADD    AL,AL
        ADD    AL,AL
        ADD    AL,AL
        ADD    AL,TBUF                 ;add page offset and..
        MOV    BL,AL                   ;..put address into..
        MOV    BH,0                    ;..hl-pair.
        CALL HB_FLAGS
        JNC HB_KEEP
        JMP HB_SEARCH_NEXT
HB_KEEP:
        MOV    AL,Byte Ptr .FCB        ;get drive/user designator and..
        MOV    M,AL                    ;..put into 'fcb' buffer.
        XCHG   BX,DX
        MOV    BX,Word Ptr RINGPOS     ;pointer to current load point in ring
        XCHG   BX,DX
        MOV    CH,12                   ;move drive designator and name to ring
        CALL   MOVE
        XCHG   BX,DX                   ;de-pair contains next load point address
        MOV    M,' '                   ;space for potential..
        LAHF                           ;..tagging of files for mass copy.
        INC    BX
        SAHF
        MOV AL,Byte Ptr HB_LAST
        MOV M,AL
        INC BX
        MOV    Word Ptr RINGPOS,BX     ;store and search..
HB_SEARCH_NEXT:
        MOV    CL,SRCHN                ;..for next occurrence.
        MOV    DX,FCB                  ;filename address field
        INT    224
        INC    AL                      ;if all done, 0ffh --> 00h.
        JNZ    TO@RING                 ;if not, put next name into ring.
; all filenames in ring -- setup ring size and copy-buffer start point
        MOV    BX,Word Ptr RINGPOS     ;next load point of ring is start of buffer
        MOV    Word Ptr RINGEND,BX     ;set ring end..
        MOV    Word Ptr BUFSTART,BX    ;..and copy-buffer start.
        CMP BX,Word Ptr RING
        JNE HB_HAVE_FILES
        JMP HB_EMPTY
HB_HAVE_FILES:
        PUSH   BX
        MOV    BX,Word Ptr RING
        MOV    DX,HB_STRIDE                   ;compare 'ringend' (tab base+13)
        LAHF
        ADD    BX,DX
        RCR    SI,1
        SAHF
        RCL    SI,1
        XCHG   BX,DX
        POP    BX
        CALL   CMPDEHL
        JNZ    L_A6    
        JMP    CMDLOOP                 ;go to command loop, if no sort.
L_A6:
; sort ring of filenames
SORT:
        MOV    BX,Word Ptr RING        ;initialize 'i' sort variable and..
        MOV    Word Ptr RINGI,BX
        MOV    DX,HB_STRIDE                   ;..also 'j' variable.
        ADD    BX,DX
        MOV    Word Ptr RINGJ,BX
SORTLP:
        MOV    BX,Word Ptr RINGJ       ;compare names 'i & j'
        XCHG   BX,DX
        MOV    BX,Word Ptr RINGI
        PUSH   BX                      ;save position pointers..
        PUSH   DX                      ;..for potential swap.
        MOV    CH,HB_STRIDE                   ; # of characters to compare
; left to right compare of two strings (de-pair points to 'a' string;
; hl-pair, to 'b'; b-reg contains string length.)
CMPSTR:
        MOV    SI,DX                   ;get an 'a' string character and..
        MOV    AL,[SI]
        CMP    AL,M                    ;..check against 'b' string character.
        JNZ    NOCMP                   ;if not equal, set flag.
        INC    BX                      ;bump compare..
        INC    DX                      ;..pointers and..
        DEC    CH                      ; (if compare, set as equal.)
        JNZ    CMPSTR                  ;..do next character.
NOCMP:
        POP    DX
        POP    BX
        MOV    CH,HB_STRIDE
        JNB    NOSWAP
; swap if 'j' string larger than 'i'
SWAP:
        MOV    CL,M                    ;get character from one string..
        MOV    SI,DX                   ;..and one from other string.
        MOV    AL,[SI]
        MOV    M,AL                    ;second into first
        MOV    AL,CL                   ;first into second
        MOV    SI,DX
        MOV    [SI],AL
        INC    BX                      ;bump swap pointers
        INC    DX
        DEC    CH
        JNZ    SWAP
NOSWAP:
        MOV    BX,Word Ptr RINGJ       ;increment 'j' pointer
        MOV    DX,HB_STRIDE
        LAHF
        ADD    BX,DX
        SAHF
        MOV    Word Ptr RINGJ,BX
        XCHG   BX,DX                   ;see if end of 'j' loop
        MOV    BX,Word Ptr RINGEND
        CALL   CMPDEHL
        JNZ    SORTLP                  ;no, so more 'j' looping.
        MOV    BX,Word Ptr RINGI       ;bump 'i' pointer
        MOV    DX,HB_STRIDE
        LAHF
        ADD    BX,DX
        SAHF
        MOV    Word Ptr RINGI,BX
        LAHF                           ;set start over 'j' pointer
        ADD    BX,DX
        SAHF
        MOV    Word Ptr RINGJ,BX
        XCHG   BX,DX                   ;see if end of 'i' loop
        MOV    BX,Word Ptr RINGEND
        CALL   CMPDEHL
        JZ     L_A7    
        JMP    SORTLP                  ;must be more 'i' loop to do
L_A7:
; sort done -- initialize tables for fast crc calculations
        CALL   INITCRC
; calculate buffer maximum available record capacity
B@SIZE:
        MOV    CX,0                    ;count records
        MOV    BX,Word Ptr .BDOS+1     ;get mem. allow from page one
        DEC    BX
        XCHG   BX,DX                   ;de-pair --> highest address of buffer
        MOV    BX,Word Ptr BUFSTART    ;start address of buffer (end of ring list)
B@SIZE2:
        INC    CX                      ;increase record count by one
        PUSH   DX
        MOV    DX,128                  ; 128-byte record
        ADD    BX,DX                   ;buffer address + record size
        POP    DX
        JC     B@SIZE4                 ;wrapped past 64k, so beyond top of memory
        CALL   CMPDEHL                 ;compare for all done
        JNB    B@SIZE2                 ;more will fit?
B@SIZE4:
        DEC    CX                      ;set maximum record count less one
        MOV    AL,CH                   ;memory available for copy?
        OR     AL,CL
        JNZ    B@SIZE3                 ;yes, buffer memory space available.
        CALL   ERMSG
        DB     'No Memory for Copy Buffer',0
        JMP    NEUTRAL
B@SIZE3:
        MOV    BL,CL                   ;store..
        MOV    BH,CH                   ;..maximum..
        MOV    Word Ptr REC@MAX,BX     ;..record count.
        JMP    CMDLOOP

; buffer size suitable -- process file/display loop
LOOPFN:
        MOV    BX,FNADR                ;position cursor for file name print
 IF     (ENH@VID AND (NOT HIDDEN))
        DEC    BX                      ;allow video attribute space
 ENDIF
        CALL   GOTOXY
        MOV    BX,Word Ptr RINGPOS     ;pt to current file name
        LAHF                           ;pt to first char
        INC    BX
        SAHF
        CALL   STAT@VID
        CALL   PRFN                    ;print file name
        CALL HB_SELECTED_FLAGS
        CALL   STD@VID
LOOP:
        CALL   ATCMD                   ;position at command prompt
        CALL   DKEYIN                  ;wait for character from keyboard
        LAHF                           ;save command
        XCHG   AL,AH
        PUSH   AX
        MOV    AL,Byte Ptr ERMFLG      ;error message?
        OR     AL,AL                   ;0=no
        JZ     CPROC
        CALL   ERCLR                   ;erase old error message
CPROC:
        POP    AX                      ;get command
        XCHG   AL,AH
        SAHF

        CALL   CTPROC                  ;process command or return if not found

        CALL   ERMSG
        DB     'Invalid Command: ',0
        MOV    AL,CH                   ;get char
        CMP    AL,' '                  ;expand if less than space
        JNB    CPROC1
        MOV    AL,'^'                  ;control
        CALL   TTYPE
        MOV    AL,CH                   ;get byte
        ADD    AL,'@'                  ;convert to letter
CPROC1:
        CALL   TTYPE
NEUTRAL:
        JMPS   LOOP                    ;..position.

;*changed - process command from table        
CTPROC:
        MOV    BX,(Offset FTABLE)      ; new CHR_LO check   *added -km 
        MOV    CH,CHR_HI               ; ah=chr      if fkey or keypad 
        CMP    CHR_LO,0                ; flag=0      if last byte was 0 
        JZ     CTPR1                   ; yes, process FTABLE 
        MOV    BX,(Offset CTABLE)      ; no,  process standard CTABLE
        MOV    CH,AL                   ; command in B
CTPR1:
        MOV    AL,M                    ; get table command char
        OR     AL,AL                   ; end of table?
        JNZ    L_A8    
        RET                            ;done if so
L_A8:
        CMP    AL,CH                   ;match?
        JZ     CTPR2
        INC    BX                      ;skip to next entry
        INC    BX
        INC    BX
        JMPS   CTPR1
CTPR2:
        LAHF                           ;pt to address
        INC    BX
        SAHF
        MOV    AL,M                    ;get low
        LAHF
        INC    BX
        SAHF
        MOV    BH,M                    ;get high
        MOV    BL,AL
        MOV    BP,SP                   ;address on stack
        XCHG   BX,[BP]
        RET                            ;"jump" to routine
L_A9    EQU     $
        DSEG
        ORG    Offset L_A9

; Command Table Lookup, menu choice
; --------------------------------------------------------------------
FTABLE  DB     HOME@KEY                ;user cursor positioning
        DW     (Offset REVERSE)
        DB     END@KEY                 ;user cursor positioning
        DW     (Offset FORWARD)
        DB     ARROW@UP                 ;user cursor positioning
        DW     (Offset UP)
        DB     ARROW@DOWN
        DW     (Offset DOWN)
        DB     ARROW@RIGHT
        DW     (Offset FORWARD)
        DB     ARROW@LEFT
        DW     (Offset REVERSE)
        DB     PG@UP
        DW     (Offset JUMPF)          ;Prev Directory page
        DB     PG@DN
        DW     (Offset JUMPB)          ;Next Directory page
        DB     0                       ;end of CHR_LO table
F_A10   EQU    $
        CSEG
        ORG    Offset F_A10


; Command Table Lookup, menu choice
; --------------------------------------------------------------------
CTABLE DB 'O'
        DW Offset HB_TOGGLE
        DB     CTRLE                   ;system cursor positioning
        DW     (Offset UP)
        ;DB     CTRLM                  ;CR *added -km
        ;DW     (Offset VIEW)
        DB     CTRLX
        DW     (Offset DOWN)
        DB     CTRLD
        DW     (Offset FORWARD)
        DB     CTRLP
        DW     (Offset LSTFILE)        ;ctrl-p to print     *added -km
        DB     CTRLS
        DW     (Offset REVERSE)
        DB     LF                      ;PgDn sends a bare line feed
        DW     (Offset JUMPB)
        DB     1AH                     ;End sends a bare ctrl-Z
        DW     (Offset FORWARD)
        DB     '+'                     ;jump forward
        DW     (Offset JUMPF)
        DB     '-'                     ;jump backward
        DW     (Offset JUMPB)
        DB     ' '                     ;go forward
        DW     (Offset FORWARD)
        DB     '.'                     ;go forward
        DW     (Offset FORWARD)
        DB     '>'                     ;go forward
        DW     (Offset FORWARD)
        DB     ','                     ;back up?
        DW     (Offset REVERSE)
        DB     '<'                     ;back up?
        DW     (Offset REVERSE)
        DB     'C'                     ;copy a file?
        DW     (Offset COPY)
        DB     'D'                     ;delete a file?
        DW     (Offset DELETE)
        DB     'E'                     ;enter a command?
        DW     (Offset RUN@CMD)
        DB     'F'                     ;show file size?
        DW     (Offset FIL@SIZ)
        DB     'G'                     ;goto a file?
        DW     (Offset GOTO)
        DB     'H'                     ;external help?
        DW     (Offset EXTHELP)
        DB     'I'                     ;
        DW     (Offset FIL@SIZ)        ;
        DB     'L'                     ;log-in another drive?
        DW     (Offset LOG)
        DB     'M'                     ;tagged multiple file copy?
        DW     (Offset MASS@COPY)
        DB     'N'                     ;new screen
        DW     (Offset SCREFRESH)
       ; DB     'P'                     ;output file to 'list' device?
       ; DW     (Offset LSTFILE)        ;*commented out -km
        DB     'Q'                     ;quit
        DW     (Offset QUIT)
        DB     'R'                     ;if rename, get to work.
        DW     (Offset RENAME)
        DB     'S'                     ;free bytes on..
        DW     (Offset R@DR@ST)        ;..requested drive?
        DB     'T'                     ;if tag, put '*' in..
        DW     (Offset TAG@EM)         ;..front of cursor.
        DB     'U'                     ;remove '*' from..
        DW     (Offset UNTAG)          ;..in front of cursor?
        DB     'W'                     ;mass tag/untag?
        DW     (Offset MASS@TU)
        DB     'V'                     ; 'view' file at console?
        DW     (Offset VIEW)
        DB     'X'                     ;execute command file
        DW     (Offset XQT@CMD)
        DB     'Y'                     ;mass delete?
        DW     (Offset MASS@DEL)
        DB     ESC                     ; 'esc' quits to cp/m ccp also.
        DW     (Offset QUIT)
        DB     '?'                     ;help
        DW     (Offset HELP)
        DB     '/'                     ;help also
        DW     (Offset HELP)
        DB     0                       ;end of table
L_A10   EQU    $
        CSEG
        ORG    Offset L_A10


; h e l p  (menu)
