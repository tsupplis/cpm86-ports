MASS@DEL:
        CALL   CPRMPT
        DB     'Mass Delete (Y/N/V=Verify Each)? ',0
        CALL   KEYIN                   ;get response
        CMP    AL,'Y'
        JZ     MD1
        CMP    AL,'V'

        JNZ    NEUT_J                  ;return to position
        JMPS   MD1
NEUT_J:        JMP     NEUTRAL                 ;too far for a jmps

MD1:
        MOV    Byte Ptr MDFLG,AL       ;set flag
        XOR    AL,AL                   ;set for mass delete
        MOV    Byte Ptr MFLAG,AL
        MOV    BX,Word Ptr RING
        MOV    Word Ptr RINGPOS,BX     ;set ring position
MD@LP:
        MOV    BX,Word Ptr RINGPOS     ;get current position
        MOV    DX,12                   ;pt to tag
        ADD    BX,DX
        MOV    AL,M                    ;get tag
        CMP    AL,'*'
        JNZ    MD@LOOP
        CALL   RINGFCB                 ;set up file name
        MOV    AL,Byte Ptr MDFLG       ;verify?
        CMP    AL,'V'
        JZ     MDEL1                   ;delete with verify
        JMP    DEL1                    ;delete without verify
MD@LOOP:
        MOV    BX,Word Ptr RINGPOS     ;re-entry point for next file mass-copy
        MOV    DX,HB_STRIDE                   ;advance to next
        LAHF
        ADD    BX,DX
        RCR    SI,1
        SAHF
        RCL    SI,1
        MOV    Word Ptr RINGPOS,BX
MD1@LOOP:
        XCHG   BX,DX                   ;at ring..
        MOV    BX,Word Ptr RINGEND     ;..end yet?
        CALL   CMPDEHL                 ; (compare present position with end)
        JNZ    MD@LP                   ;no, loop 'till thru ring list.
MD@EXIT:
        MOV    AL,TRUE                 ;set no
        MOV    Byte Ptr MFLAG,AL       ;..mass-delete request.
        JMP    CMDLOOP                 ;jump to 'ring' beginning
; set up to delete filename at cursor position
DELETE:
        MOV    AL,TRUE                 ;set for no mass delete
        MOV    Byte Ptr MFLAG,AL
        MOV    Byte Ptr MDFLG,AL
MDELETE:
        CALL   RINGFCB                 ;move file name
MDEL1:
        CALL   CPRMPT
        DB     'Delete ',0
        CALL   PRFNS                   ;print file name in S$FCB
        CALL   ILPRT
        DB     ' (Y/N)? ',0
        CALL   KEYIN
        CMP    AL,'Y'
        JZ     DEL1
        MOV    AL,Byte Ptr MFLAG       ;mass delete?
        OR     AL,AL
        JZ     MD@LOOP
MDEL2:
        MOV    BX,Word Ptr LOCEND      ;move in end
        MOV    DX,-HB_STRIDE
        LAHF
        ADD    BX,DX
        RCR    SI,1
        SAHF
        RCL    SI,1
        MOV    Word Ptr LOCEND,BX
        XCHG   BX,DX
        MOV    BX,Word Ptr RINGPOS     ;position beyond end of ring?
        CALL   CMPDEHL
        JNZ    MDEL3
        CALL   CUR@BACK                ;back up cursor
        MOV    BX,Word Ptr LOCEND      ;reset position
        MOV    DX,-HB_STRIDE
        LAHF
        ADD    BX,DX
        RCR    SI,1
        SAHF
        RCL    SI,1
        MOV    Word Ptr RINGPOS,BX
        MOV    BX,Word Ptr LOCEND      ;get end
        XCHG   BX,DX
MDEL3:
        MOV    BX,Word Ptr LOCBEG      ;erased all local files?
        CALL   CMPDEHL
        JNZ    L_B3    
        JMP    CMDLOOP                 ;reset
L_B3:
        JMP    JFW0A                   ;rescreen
; delete file
DEL1:
        MOV    AL,Byte Ptr MDFLG       ;Y option on Mass Delete?
        CMP    AL,'Y'
        JNZ    DEL1A
        CALL   ERMSG
        DB     'Deleting File ',0
        CALL   PRFNS
DEL1A:
        MOV    BX,(Offset S@FCB)       ;set file to R/W
        CALL   ATTRIB
        MOV    DX,(Offset S@FCB)       ;point at delete 'fcb'
        MOV    CL,ERASE                ;erase function
        INT    224
        INC    AL
        JZ     FNF@MSG                 ;print error message
        CALL   DEL2                    ;close up erased position
        MOV    AL,Byte Ptr MFLAG       ;check for mass delete
        OR     AL,AL
        JZ     L_B4    
        JMP    MDEL2
L_B4:
        MOV    BX,Word Ptr RINGPOS     ;no advance because of close up
        JMP    MD1@LOOP
FNF@MSG:
        CALL   ERMSG                   ;show error message
        DB     'No File Found',0
        JMP    LOOPFN
; reverse ring to close up erased position
DEL2:
        MOV    BX,Word Ptr RINGPOS     ;prepare move up pointers
        PUSH   BX
        MOV    DX,HB_STRIDE                   ;13 bytes/entry
        LAHF                           ;de-pair = 'to' location
        ADD    BX,DX
        RCR    SI,1
        SAHF
        RCL    SI,1
        POP    DX                      ;hl-pair = 'from' location
MOVUP:
        XCHG   BX,DX
        PUSH   BX                      ;check if at end
        MOV    BX,Word Ptr RINGEND     ;get old end pointer
        CALL   CMPDEHL                 ;check against current end location
        POP    BX
        XCHG   BX,DX
        JZ     MOVDONE                 ;must be at end of ring
        MOV    CH,HB_STRIDE                   ;one name size
        CALL   MOVE                    ;move one name up
        JMPS   MOVUP                   ;go check end parameters
MOVDONE:
        MOV    BX,Word Ptr RING        ;see if ring is empty
        XCHG   BX,DX
        MOV    Word Ptr RINGEND,BX     ;set new ring end if all moved
        CALL   CMPDEHL                 ;..(listend --> listpos --> ring)
        JZ     L_B5
        RET
L_B5:
        MOV    BX,Word Ptr RINGPOS
        CALL   CMPDEHL
        JZ     L_B6    
        RET                            ;neither equal so not empty
L_B6:
        MOV    SP,(Offset STACK)       ;reset stack
        CALL   ERMSG
        DB     'List Empty',0
        MOV    AL, 0FFh
        MOV    Byte Ptr CANFLG,AL      ;make log-cancel toggle false
        JMP    LOG                     ;go to drive/user area with files
; r e n a m e
; set-up to rename file at cursor position -- scan keyboard buffer and
; move filename to 'rename' destination 'fcb' (dfcb)
RENAME:
        MOV    BX,Word Ptr RINGPOS     ;move name from ring to rename 'fcb'
        MOV    DX,(Offset D@FCB)       ;place to move name
        MOV    CH,12                   ;amount to move
        CALL   MOVE
        CALL   CPRMPT                  ;new name prompt
        DB     'Rename File to: ',0
        MOV    DX,(Offset D@FCB)+16    ;pt to FCB to fill
        CALL   FILENAME                ;get file name
        MOV    BX,(Offset D@FCB)+1     ;check for any wild cards -- none permitted
        MOV    CH,11                   ;11 bytes
WILDCHK:
        MOV    AL,M                    ;get char
        INC    BX                      ;pt to next
        CMP    AL,'?'                  ;wild?
        JZ     WILDFND
        DEC    CH
        JNZ    WILDCHK
; copy old file status bit ($r/o or $sys) to new filename
CPYBITS:
        MOV    DX,(Offset D@FCB)+1     ;first character of old name..
        MOV    BX,(Offset D@FCB)+17    ;..and of new name.
        MOV    CH,11                   ; # of bytes with tag bits
CBITS1:
        MOV    SI,DX                   ;fetch bit of old name character
        MOV    AL,[SI]
        AND    AL,128                  ;strip upper bit and..
        MOV    CL,AL                   ;..save in b-reg.
        MOV    AL,7FH                  ;mask for character only
        AND    AL,M                    ;put masked character into a-reg
        OR     AL,CL                   ;add old bit
        MOV    M,AL                    ;copy new byte back
        LAHF                           ;bump copy pointers
        INC    BX
        SAHF
        LAHF
        INC    DX
        SAHF
        DEC    CH
        JNZ    CBITS1
; check if new filename already exists.  if so, say so.  then go
; to command loop without moving ring position
        MOV    AL,Byte Ptr D@FCB       ;copy new name to source 'fcb'
        MOV    Byte Ptr S@FCB,AL
        MOV    CH,11
        MOV    BX,(Offset D@FCB)+17    ;copy new name to..
        MOV    DX,(Offset S@FCB)+1     ;..source 'fcb' for existence check.
        CALL   MOVE
        MOV    BX,(Offset S@FCB)+12    ;clear cp/m 'fcb' system..
        CALL   INITFCB                 ;..fields.
        MOV    DX,(Offset S@FCB)       ;search to see if this file exists
        MOV    CL,SRCHF                ;search first function
        INT    224
        INC    AL                      ; 0ffh --> 00h if file not found
        JZ     RENFILE                 ;to rename, if duplicate doesn't exists.
        CALL   ERMSG                   ;announce the situation
        DB     'File Already Exists',0
        JMP    COMCAN                  ;try again?
; wild char found in file name -- error
WILDFND:
        CALL   ERMSG
        DB     'Ambiguous File Name NOT Allowed',0
        JMP    COMCAN
; copy new name into ring position
RENFILE:
        MOV    BX,Word Ptr RINGPOS     ;get ring position pointer
        LAHF                           ;pt to name
        INC    BX
        SAHF
        PUSH   BX                      ;save ptr
        XCHG   BX,DX
        MOV    BX,(Offset D@FCB)+17    ;point at new name and..
        MOV    CH,11
        CALL   MOVE                    ;..move.
        MOV    BX,Word Ptr CURAT       ;get current position on screen
        MOV    DX,4                    ;advance 4 chars
        LAHF
        ADD    BX,DX
        RCR    SI,1
        SAHF
        RCL    SI,1
        CALL   GOTOXY
        POP    BX                      ;get ptr
        CALL   AC@PRFN                 ;print file name
        MOV    DX,(Offset D@FCB)       ;rename 'fcb' location
        MOV    CL,REN                  ;rename function
        INT    224
        INC    AL                      ; 0ffh --> 00h if rename error
        JZ     L_B7    
        JMP    NEUTRAL                 ;if okay, proceed, else..
L_B7:
        JMP    FNF@MSG                 ;..show no-file msg.
; get file name from user and process into FCB pted to by DE
FILENAME:
        PUSH   DX                      ;save ptr
        MOV    DX,(Offset CMDBUF)      ;command line location
        MOV    CL,RDBUF                ;console read-buffer function
        INT    224
        CALL   CONVERT                 ;capitalize alpha
        POP    BX                      ;set to null drive
        MOV    M,0                     ;..required by 'bdos'.
        LAHF
        INC    BX
        SAHF
; initialize new filename field with spaces
        PUSH   BX                      ;save start pointer
        MOV    CH,11                   ; # of spaces to 'blank'
        CALL   FILL
        POP    BX
        XCHG   BX,DX
        MOV    BX,(Offset CMDBUF)+1    ;put length..
        MOV    CL,M                    ;..in c-reg.
        LAHF
        INC    BX
        SAHF
        XCHG   BX,DX                   ;de-pair --> buffer pointer and hl-pair..
        CALL   UNSPACE                 ;..--> 'fcb' pointer.  remove leading spaces.
; extend buffer to spaces beyond command length
EXTEND:
        PUSH   BX
        MOV    BL,CL                   ;double-byte remaining length
        MOV    BH,0
        LAHF                           ;to buffer end +1
        ADD    BX,DX
        RCR    SI,1
        SAHF
        RCL    SI,1
        MOV    M,' '                   ;force illegal character end
        POP    BX
; start filename scan
SCAN:
        MOV    CH,8                    ; 8 characters in filename
SCAN1:
        CALL   CKLEGAL                 ;get and see if legal character
        JNB    L_B8    
        JMP    COMCAN                  ;all of command line?
L_B8:
        CMP    AL,' '                  ;see if end of parameter field
        JNZ    L_B9    
        RET                            ;rename file
L_B9:
        CMP    AL,'.'                  ;at end of filename
        JZ     SCAN2                   ;process filetype field
        CMP    AL,'*'                  ;rest wild?
        JZ     SCAN1B
        MOV    M,AL                    ;put character into destination 'fcb'
        LAHF
        INC    BX
        SAHF
        DEC    CH
        JNZ    SCAN1
; entry if eight characters without a 'period'
SCAN1A:
        CALL   CKLEGAL                 ;scan buffer up to period or end
        JNB    L_B10   
        RET                            ;no extent if not legal
L_B10:
        CMP    AL,' '                  ;end of parameter field?
        JNZ    L_B11
        RET
L_B11:
        CMP    AL,'.'
        JNZ    SCAN1A                  ;do till end or period
        JMPS   SCAN2A                  ;continue at correct place
; make rest of entry wild
SCAN1B:
        MOV    M,'?'                   ;fill with ?'s
        INC    BX
        DEC    CH
        JNZ    SCAN1B
        MOV    SI,DX                   ;get next char
        MOV    AL,[SI]
        INC    DX                      ;pt to after dot
        CMP    AL,'.'                  ;must be dot
        JZ     L_B12   
        JMP    COMCAN                  ;cancel if not
L_B12:
        JMPS   SCAN2A
; build filetype field
SCAN2:
        LAHF                           ;advance ptr to file type field
        INC    BX
        SAHF
        DEC    CH
        JNZ    SCAN2
SCAN2A:
        MOV    CH,3                    ;length of filetype field
SCAN3:
        CALL   CKLEGAL                 ;get and check character
        JB     SCAN4                   ;name done if illegal
        CMP    AL,' '                  ;end of parameter field?
        JZ     SCAN4
        CMP    AL,'.'                  ;check if another period
        JZ     SCAN4
        CMP    AL,'*'                  ;rest wild?
        JZ     SCAN4B
        MOV    M,AL
        LAHF
        INC    BX
        SAHF
        DEC    CH
        JNZ    SCAN3                   ;get next character
        JMPS   SCAN4A
SCAN4:
        LAHF                           ;advance to end of type field
        INC    BX
        SAHF
        DEC    CH
        JNZ    SCAN4
SCAN4A:
        CALL   INITFCB                 ;..and zero counter fields.
        RET
SCAN4B:
        MOV    M,'?'                   ;make wild
        LAHF
        INC    BX
        SAHF
        DEC    CH
        JNZ    SCAN4B
        JMPS   SCAN4A                  ;complete rest
; goto file
GOTO:
        CALL   CPRMPT
        DB     'Goto Filename: ',0
        MOV    DX,(Offset D@FCB)       ;pt to FCB
        CALL   FILENAME                ;get file name
        MOV    BX,Word Ptr RING        ;pt to first element of ring
        MOV    Word Ptr RINGPOS,BX     ;set position
        MOV    Word Ptr LOCBEG,BX      ;set local beginning
        XOR    AL,AL                   ;set local counter
        MOV    Byte Ptr CRCTBL,AL      ;use this buffer
GOTOL:
        CALL   GOTOCOMP                ;compare
        JZ     GOTOF                   ;we are there
        MOV    AL,Byte Ptr CRCTBL      ;increment count
        INC    AL
        MOV    Byte Ptr CRCTBL,AL
        CMP    AL,EPS
        JNZ    GOTOL1
        XOR    AL,AL                   ;reset count
        MOV    Byte Ptr CRCTBL,AL
        MOV    BX,Word Ptr LOCBEG      ;reset local beginning
        MOV    DX,EPS*HB_STRIDE
        LAHF
        ADD    BX,DX
        SAHF
        MOV    Word Ptr LOCBEG,BX
GOTOL1:
        MOV    BX,Word Ptr RINGPOS     ;advance to next entry
        MOV    DX,HB_STRIDE
        LAHF
        ADD    BX,DX
        RCR    SI,1
        SAHF
        RCL    SI,1
        MOV    Word Ptr RINGPOS,BX     ;new position
        XCHG   BX,DX                   ;position in DE
        MOV    BX,Word Ptr RINGEND     ;check for completion
        CALL   CMPDEHL                 ;compare current position with end of ring
        JNZ    GOTOL
        MOV    BX,Word Ptr RING        ;pt to first element
        MOV    Word Ptr RINGPOS,BX     ;set position
        CALL   ERMSG
        DB     'File NOT Found',0
        JMP    CMDLOOP
GOTOF:
        MOV    BX,Word Ptr LOCBEG      ;we have local beginning
        PUSH   BX
        XCHG   BX,DX                   ;ring location in DE
        MOV    BX,CURHOME              ;set cursor ptr
        MOV    Word Ptr CURAT,BX
GOTOF0:
        CALL   CUR@OFF                 ;turn off term. cursor
GOTOF0A:
        MOV    BX,Word Ptr RINGPOS     ;at position?
        CALL   CMPDEHL
        JZ     GOTOF1
        MOV    BX,HB_STRIDE                   ;advance location
        LAHF
        ADD    BX,DX
        RCR    SI,1
        SAHF
        RCL    SI,1
        PUSH   BX
        CALL   CUR@NEXT                ;advance cursor
        POP    DX                      ;pt to next ring position
        JMPS   GOTOF0A
GOTOF1:
        CALL   CUR@ON                  ;turn on term. cursor
        POP    BX                      ;pt to local ring
        JMP    JFW0A                   ;process
GOTOCOMP:
        MOV    BX,Word Ptr RINGPOS     ;pt to current entry
        INC    BX                      ;pt to first char of file name
        MOV    DX,(Offset D@FCB)+1     ;pt to first char of new file
        MOV    CH,11                   ;11 bytes
GOTOC1:
        MOV    SI,DX                   ;get char
        MOV    AL,[SI]
        CMP    AL,'?'                  ;match?
        JZ     GOTOC2
        CMP    AL,M                    ;match?
        JZ     L_B13   
        RET                            ;no match
L_B13:
GOTOC2:
        LAHF                           ;pt to next
        INC    DX
        SAHF
        LAHF
        INC    BX
        SAHF
        DEC    CH
        JNZ    GOTOC1
        RET

; v i e w
; type file to console, pagination set to 'lps' - single-line scroll
; using <space> bar , <ctrl-x> to cancel, any other key to page screen.
VIEW:
        CALL   CHK@CMD                 ;check if CMD filetype
        JNZ    OK_VIEW                 ;give error if CMD type
        CALL   ERMSG
        DB     'Can not View CMD type files',0
        JMP    LOOPFN
OK_VIEW:
        CALL   CLS
        CALL   ILPRT
        DB     CR,LF,0                ;header msg *commented out -km 
        MOV    AL,1                    ;initialize..
        MOV    Byte Ptr LPSCNT,AL      ;..lines-per-screen counter.
        MOV    Byte Ptr VIEWFLG,AL     ; 'view' paginate if not zero
        MOV    AL,WRCON                ;write console out function
        JMP    CURRENT                 ;to common i/o processing
; p r i n t e r
; send file to logical list device -- any keypress cancels
LSTFILE:
        CALL   CPRMPT
        DB     'Print current file (Y/N)? ',0
        CALL   KEYIN                   ;get response
        CMP    AL,'Y'
        JE     L_B14   
        JMP    NEUTRAL
L_B14:
        CALL   CHK@CMD                 ;check if CMD filetype
        JNZ    OK_LST                  ;give error if CMD type
        CALL   ERMSG
        DB     'Can not Print CMD type files',0
        JMP    LOOPFN
OK_LST:
        CALL   RDTIME                  ;read clock
        MOV    AL,Byte Ptr LTPP        ;get lines to print per page
        MOV    Byte Ptr LTPP1,AL       ;stash
        MOV    Byte Ptr HEADFLG,TRUE   ;set flag defaults
        MOV    Byte Ptr PAGEFLG,TRUE
        CALL   ERMSG
        DB     '*** Make Printer Ready ***',0
        CALL   CPRMPT
        DB     'Page the printer output (Y/N)? ',0
        CALL   KEYIN
        CMP    AL,'Y'
        JE     L_B14A
        MOV    Byte Ptr PAGEFLG,FALSE
        JMPS   L_B14B
L_B14A:
        CALL   CPRMPT
        DB     'Print Header on every page (Y/N)? ',0
        CALL   KEYIN
        CMP    AL,'Y'
        JE     L_B14B
        MOV    HEADFLG,FALSE           ;set flag false
L_B14B:
        CALL   ERMSG
        DB     'Printing ',0
        MOV    BX,Word Ptr RINGPOS     ;pt to file name
        LAHF
        INC    BX
        SAHF
        CALL   PRFN                    ;print it
        CALL   CPRMPT
        DB     'Press ESC, or Q key to quit printing ',0

        MOV    AL,1                    ;one for..
        MOV    Byte Ptr VIEWFLG,AL     ;..output to printer.
        DEC    AL                      ;zero for..
        MOV    Byte Ptr LPSCNT,AL      ;..lines-per-page counter
        MOV    AL,LLIST                ;out to 'list' device function and fall thru
; output character for console/list/punch processing
CURRENT:
        MOV    Byte Ptr CON@LST,AL     ;save bdos function
; output file to console/printer/punch
        CALL   RINGFCB                 ;position name to 'fcb'
        XCHG   BX,DX                   ;HL pts to S$FCB
        CALL   INITFCB                 ;set 'fcb' for use
        MOV    DX,TBUF                 ;set to use default cp/m dma buffer
        MOV    CL,SETDMA               ;address set function
        INT    224
        MOV    DX,(Offset S@FCB)       ;open file for reading
        MOV    CL,OPEN                 ;file open function code
        INT    224
        INC    AL                      ; 0ffh --> 00h if open not okay
        JNZ    ZEROCR                  ;if not okay, show error message.
        CALL   ERMSG
        DB     'Unable to Open File',0
        JMP    NEUTRAL
ZEROCR:
        XOR    AL,AL
        MOV    Byte Ptr S@FCB+32,AL    ;zero file 'current record' field
        MOV    Byte Ptr CHARCNT,AL     ;zero char count for tabbing
        CALL   PHEAD                   ;print heading if output to LST device
READMR:
        MOV    DX,(Offset S@FCB)       ;point at file 'fcb' for reading
        MOV    CL,READ                 ;record read function
        INT    224
        OR     AL,AL                   ;check if read okay
        JZ     L_B15   
        JMP    CURDONE                 ;eof?
L_B15:
        MOV    BX,TBUF                 ;point at record just read
        MOV    CH,128                  ;set record character counter to output
READLP:
        MOV    AL,M                    ;get a character
        AND    AL,7FH                  ;force to 'ascii'
        CMP    AL,EOFCHAR              ;see if end-of-file
        JNZ    L_B16   
        JMP    CURDONE                 ;back to ring loop if 'eof'
L_B16:
        MOV    DL,AL                   ;put character for 'bdos' call
        PUSH   CX
        PUSH   BX
        PUSH   DX                      ; (character in e-reg)
        MOV    AL,Byte Ptr CON@LST     ;get function for punch/list/console output
        MOV    CL,AL
        MOV    AL,DL                   ;check char
        CMP    AL,TAB                  ;tabulate?
        JNZ    NOTAB
        MOV    DL,' '                  ;space over
TABL:
        PUSH   CX                      ;save key regs
        PUSH   DX
        INT    224
        POP    DX                      ;get key regs
        POP    CX
        CALL   INCCCNT                 ;increment char count
        AND    AL,7                    ;check for done at every 8
        JNZ    TABL
        JMPS   TABDN
NOTAB:
        INT    224                     ;send character
        CALL   INCCCNT                 ;increment char count
TABDN:
        MOV    AL,Byte Ptr VIEWFLG     ;if 'view'..
        OR     AL,AL
        POP    DX                      ;get char in E in case PAGER is called
        JZ     L_B17   
        CALL   PAGER                   ;..check for 'lf'.
L_B17:
        MOV    DL,GET                  ;get status or char
        MOV    CL,DIRCON               ;console status function
        INT    224                     ;status?
        POP    BX
        POP    CX
        AND    AL,7FH                  ;if character there, then abort..
        JZ     L_B18   
        CALL   CHK@CAN                 ;already got char
L_B18:
        INC    BX                      ;if not, bump buffer pointer.
        DEC    CH
        JZ     L_B19   
        JMP    READLP                  ;no, more in present record.
L_B19:
        JMP    READMR                  ;yes, get next record.
CURDONE:
        MOV    AL,Byte Ptr CON@LST     ;console?
        CMP    AL,WRCON
        JNZ    L_B20   
        CALL   BOTTOM                  ;prompt for user
L_B20:
        CALL   REFRESH                 ;refresh screen
        JMP    LOOPFN
PAGER:
        MOV    AL,DL                   ; (character in e-reg)
        CMP    AL,LF
        JZ     L_B21
        RET
L_B21:
        XOR    AL,AL                   ;zero char count
        MOV    Byte Ptr CHARCNT,AL
        MOV    AL,Byte Ptr CON@LST     ;printer or console?
        CMP    AL,LLIST                ;check for printer
        JZ     PAGEP
        MOV    AL,Byte Ptr CTPP        ;get number of lines of text per screen
        MOV    CH,AL                   ;... in B
        MOV    AL,Byte Ptr LPSCNT      ;is counter..
        INC    AL                      ;..at..
        MOV    Byte Ptr LPSCNT,AL      ;..limit..
        CMP    AL,CH                   ;..of lines-per-screen?
        JNB    L_B22   
        RET                            ;no, return.
L_B22:
        XOR    AL,AL                   ;yes, initialize..
        MOV    Byte Ptr LPSCNT,AL      ;..for next screen full.
        CALL   ILPRT
        DB     CR,'[More..]',CR,0         ;show -more- msg line
        CALL   DKEYIN                  ;wait for keyboard input
        CMP    AL,' '                  ;see if <space> bar..
        LAHF
        XCHG   AL,AH
        PUSH   AX
        CALL   ILPRT
        DB     '                ',CR,0 ;clear above msg line
        POP    AX
        XCHG   AL,AH
        SAHF
        JNZ    CHK@CAN                 ;..if not, see if cancel.
        MOV    AL,Byte Ptr CTPP        ;set for single line
        DEC    AL
        MOV    Byte Ptr LPSCNT,AL      ;..scroll and..
        RET                            ;..return for one more line.

PAGEP:
        CALL   PRN@CAN                 ;check if cancel
        TEST   Byte Ptr PAGEFLG,TRUE   ;test paging flag
        JZ     PAGE_RET                ;if true then...
        MOV    AL,Byte Ptr LTPP1       ;get number of lines of text per page
        MOV    CH,AL                   ;... in B
        MOV    AL,Byte Ptr LPSCNT      ;is counter..
        INC    AL                      ;..at..
        MOV    Byte Ptr LPSCNT,AL      ;..limit..
        CMP    AL,CH                   ;..of lines-per-screen?
        JNB    L_B23   
        RET                            ;no, return.
L_B23:
        XOR    AL,AL                   ;zero for..
        MOV    Byte Ptr LPSCNT,AL      ;..lines-per-page counter
        MOV    AL,Byte Ptr LSPP        ;number of lines to skip
        MOV    CH,AL                   ;number of lines to skip
        MOV    CL,LLIST                ;LST output
PAGELST:
        CALL   LCRLF                   ;new line on LST
        DEC    CH
        JNZ    PAGELST

        TEST   Byte Ptr HEADFLG,TRUE   ;test if print header true
        JNZ    PRN_HEAD                ;if not true then...

        MOV    AL,Byte Ptr LTPP        ;get lines to print per page
        INC AL ! INC AL                        ;add 2 lines 
        MOV    Byte Ptr LTPP1,AL
        RET                            ;and return
PRN_HEAD:
        CALL   PHEAD                   ;...else print heading
PAGE_RET:
        RET                            ;done

PRN@CAN:                               ;check for list cancel
        CALL   CST                     ;has key been hit
        CMP    AL,0
        JNZ    KEY_HIT
        RET                            ;if not, return
KEY_HIT:
        CALL   CIN                     ;else get char
                                        ;and ... 
CHK@CAN:                               ;check for cancel
        CALL   UCASE
        CMP    AL,'Q'                  ;Q = quit
        JE     L_B24   
        CMP    AL,ESC                  ;so does ESC
        JE     L_B24
        CMP    AL,CTRLC                ;and ^C
        JE     L_B24
        RET                            ;return for another page
L_B24:
        MOV    CL,Byte Ptr CON@LST     ;get function
        MOV    DL,CR                   ;and send it a <CR>
        INT    224                     ;to flush printer buffer
        JMP    COMCAN                  ;cancel

INCCCNT:
        MOV    AL,Byte Ptr CHARCNT     ;increment char count
        INC    AL
        MOV    Byte Ptr CHARCNT,AL
        RET

PHEAD:
        MOV    AL,Byte Ptr CON@LST     ;printing to printer?
        CMP    AL,LLIST
        JZ     L_B25
        RET
L_B25:
        MOV    BX,(Offset HEADMSG)     ;print heading
        CALL   PHEAD1
PHEAD2:
        MOV    BX,(Offset S@FCB)+1     ;pt to file name
        MOV    CH,8                    ;8 chars
        CALL   PHEAD3
        MOV    AL,'.'                  ;dot
        CALL   LOUT
        MOV    CH,3                    ;3 more chars
        CALL   PHEAD3

        MOV    BX,OFFSET DATE
        CALL   PHEAD1

        CALL   LCRLF                   ;new line
        CALL   LCRLF                   ;blank line
        RET

PHEAD1:
        MOV    AL,M                    ;get char
        OR     AL,AL                   ;done?
        JZ     PH1_RET
        CALL   LOUT                    ;send to printer
        INC    BX                      ;pt to next
        JMPS   PHEAD1
PH1_RET:
        RET

PHEAD3:
        MOV    AL,M                    ;get char
        CALL   LOUT                    ;LST it
        LAHF                           ;pt to next
        INC    BX
        SAHF
        DEC    CH
        JNZ    PHEAD3
        RET

