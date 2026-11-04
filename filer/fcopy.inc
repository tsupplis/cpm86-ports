; m a s s   c o p y
; copy files tagged using the 't' command.  auto-erase if file exists
; on requested destination drive or in user area.
MASS@COPY:
        CALL   CPRMPT
        DB     'Mass Copy (Y/N/V=Verify Overwrite)? ',0
        CALL   KEYIN
        CMP    AL,'Y' ! JE MASC1
        CMP    AL,'V' ! JE MASC1
        JMP    NEUTRAL
MASC1:
        MOV    Byte Ptr MCFLG,AL       ;save answer
        MOV    BX,Word Ptr RINGPOS     ;save position
        MOV    Word Ptr SRINGPOS,BX
        MOV    BX,Word Ptr RING
        MOV    Word Ptr RINGPOS,BX     ;set position
MASS@LP:
        MOV    BX,Word Ptr RINGPOS     ;get position
        MOV    DX,12                   ;get 1st possible tag location
        ADD    BX,DX
        MOV    AL,M                    ;get tag
        CMP    AL,'*'
        JZ     MCOPY                   ;copy filename with tag character (*)
M@LP:
        MOV    BX,Word Ptr RINGPOS     ;re-entry point for next file mass-copy
        MOV    DX,HB_STRIDE                   ;advance to next
        LAHF
        ADD    BX,DX
        SAHF
        MOV    Word Ptr RINGPOS,BX
        XCHG   BX,DX                   ;at ring..
        MOV    BX,Word Ptr RINGEND     ;..end yet?
        CALL   CMPDEHL                 ; (compare present position with end)
        JNZ    MASS@LP                 ;loop 'till thru ring list.
MF@EXIT:
        XOR    AL,AL                   ;reset flags..
        MOV    Byte Ptr FIRST@M,AL     ;..for..
        NOT    AL                      ;..next..
        MOV    Byte Ptr MFLAG,AL       ;..mass-copy request.
        MOV    BX,Word Ptr SRINGPOS    ;reset ring position
        MOV    Word Ptr RINGPOS,BX
        MOV    BX,Word Ptr LOCBEG      ;local ring
        JMP    JFW0A                   ;rescreen
; c o p y
; copy source file at current 'ring' position to another drive.  set-up
; fcb's and buffer area and check for correct keyboard inputs.  contains
; auto-crc file copy verification.
MCOPY:
        XOR    AL,AL                   ;zero flag to..
        MOV    Byte Ptr MFLAG,AL       ;..mass copy.
        JMPS   COPY0M
COPY:
        MOV    Byte Ptr MCFLG,'V'      ;initialize verify flag
COPY0M:
        MOV    BX,0                    ;initialize storage for..
        MOV    Word Ptr CRCVAL,BX      ;..'crc' working value.
        CALL   RINGFCB                 ;move from 'ring' to 'sfcb'
        MOV    BX,(Offset S@FCB)+12    ;set pointer to source extent field
        CALL   INITFCB
        XOR    AL,AL                   ;zero fcb 'cr' field
        MOV    Byte Ptr S@FCB+32,AL
        MOV    CH,32                   ;copy source 'fcb' to destination 'fcb'
        MOV    BX,(Offset S@FCB)+1     ;from point..
        MOV    DX,(Offset D@FCB)+1     ;..to point..
        CALL   MOVE                    ;..move across.
        MOV    DX,(Offset S@FCB)       ;open file for reading
        MOV    CL,OPEN                 ;open function
        INT    224
        INC    AL                      ; 0ffh --> 00h if bad open
        JNZ    COPY2                   ;if okay, skip error message.
        CALL   ERMSG
        DB     'Unable to Open Source',0
        JMP    NEUTRAL

COPY2:
        MOV    AL,Byte Ptr FIRST@M     ;by-pass prompt, drive/user compatibility..
        OR     AL,AL                   ;..test, and disk reset after..
        JNZ    COPY3M                  ;..1st time thru in mass-copy mode.
        CALL   CPRMPT                  ;prompt for drive selection
        DB     'Copy to DIR: ',0
        CALL   DEF@D@U
; either drives or user areas must be different
        MOV    AL,Byte Ptr .FCB        ;get requested drive from 'fcb' and..
        MOV    CH,AL                   ;..put into b-reg for..
        MOV    AL,Byte Ptr S@FCB       ;..comparison.
        CMP    AL,CH
        JNZ    COPY3                   ;branch if different
        MOV    AL,Byte Ptr R@U@A       ;requested user area --> rua
        MOV    CH,AL
        MOV    AL,Byte Ptr C@U@A       ;current user area --> cua
        CMP    AL,CH
        JNZ    COPY3
        CALL   ERMSG                   ;if not, show error condition:
        DB     'Drives or User Areas must be different',0
        JMP    NEUTRAL                 ;try again?
COPY3:
        CALL   RESET                   ;make sure disk is read/write
COPY3M:
        CALL   ATCMD                   ;clear command line
        CALL   ERMSG 
        DB     'Copying File ',0
        MOV    BX,(Offset D@FCB)+1     ;print file name
        CALL   PRFNSX

        MOV    AL,Byte Ptr .FCB        ;put requested drive into..
        MOV    Byte Ptr D@FCB,AL       ;..place in destination fcb.
        MOV    AL,Byte Ptr R@U@A       ;toggle to..
        CALL   SET@USR                 ;..requested user area.
        MOV    DX,(Offset D@FCB)       ;search for duplicate
        MOV    CL,SRCHF                ; 'search first' function
        INT    224
        INC    AL                      ;if not found, 0ffh --> 00h.  then..
        JZ     COPY5                   ;go to 'make' function for new file.

        CMP    Byte Ptr MCFLG,'V'      ;verify or auto erase
        JNE    COPY4M                  ;..in mass-copy mode.
        CALL   CPRMPT                  ;CPR2 - if found, ask to replace:
        DB     'Overwrite Existing File (Y/N)? ',0
        CALL   KEYIN                   ;get answer
        CMP    AL,'Y'                  ;if yes, then..
        JZ     COPY4M                  ;..delete and overlay.
        CMP    Byte Ptr MFLAG,0        ;zero is mass copy
        JNZ    NOT_MASS
        MOV    Byte Ptr FIRST@M,0FFH   ;set first pass flag
        JMP    M@LP                    ;...and re-enter mass copy loop
NOT_MASS:
        MOV    AL,Byte Ptr C@U@A       ;reset to..
        CALL   SET@USR                 ;..current user area.
        JMP    FORWARD                 ;if re-copy not wanted, to next position.
COPY4M:
        MOV    BX,(Offset D@FCB)       ;pt to FCB
        CALL   ATTRIB                  ;override RO, preserving other attributes
        MOV    DX,(Offset D@FCB)       ;delete file already existing
        MOV    CL,ERASE                ;erase function
        INT    224
COPY5:
        MOV    DX,(Offset D@FCB)       ;create new file and open for writing
        MOV    CL,MAKE                 ;make function
        INT    224
        INC    AL                      ;if directory full, 0ffh --> 00h.
        JNZ    COPY6                   ;if not, branch.
        CALL   ERMSG
        DB     'Destination Directory Full',0
        JMP    LOOPFN                  ;if error, back to ring processor.
COPY6:
;      CALL    ERMSG
;      DB      'Copying File ',0
;      MOV     BX,(Offset D@FCB)+1     ;print file name
;      CALL    PRFNSX
        MOV    BX,ERADR
        ADD    BX,26                   ;position cursor after file name
        CALL   GOTOXY
        XOR    AL,AL                   ;clear 'eof'..
        MOV    Byte Ptr EOFLAG,AL      ;..flag.
COPY6A:
        MOV    AL,Byte Ptr C@U@A       ;reset user area..
        CALL   SET@USR                 ;..to current.
        MOV    BX,0                    ;clear current-record..
        MOV    Word Ptr REC@CNT,BX     ;..counter.
        MOV    BX,Word Ptr BUFSTART    ;set buffer start pointer..
        MOV    Word Ptr BUF@PT,BX      ;..to begin pointer.
; read source file -- fill buffer memory or stop on 'eof' -- update 'crc'
; on-the-fly
COPY7:
        MOV    BX,Word Ptr BUF@PT      ;set dma address to buffer pointer
        XCHG   BX,DX                   ; de-pair --> dma address
        MOV    CL,SETDMA
        INT    224
        MOV    DX,(Offset S@FCB)       ;source 'fcb' for reading
        MOV    CL,READ                 ;record read function
        INT    224
        OR     AL,AL                   ; 00h --> read okay
        JZ     S@RD@OK
        DEC    AL                      ;eof?
        JZ     COPY8                   ;yes, end-of-file, set 'eof' flag.
        CALL   ERMSG
        DB     'Source Read Error',0
        JMP    LOOPFN
S@RD@OK:
        MOV    BX,Word Ptr BUF@PT
        MOV    CH,128
COPY7A:
        MOV    AL,M                    ;get character and..
        CALL   UPDCRC                  ;..add to 'crc' value.
        INC    BX
        DEC    CH
        JNZ    COPY7A                  ;loop 'till record read finished
        MOV    BX,Word Ptr BUF@PT      ;bump buffer pointer..
        MOV    DX,128                  ;..by..
        LAHF                           ;..one..
        ADD    BX,DX
        SAHF
        MOV    Word Ptr BUF@PT,BX      ;..record.
        MOV    BX,Word Ptr REC@CNT     ;bump buffer..
        LAHF                           ;..record count and..
        INC    BX
        SAHF
        MOV    Word Ptr REC@CNT,BX     ;..store.
        XCHG   BX,DX                   ;ready to compare to..
        MOV    BX,Word Ptr REC@MAX     ;..maximum record count (full-buffer).
        CALL   CMPDEHL                 ;compare
        JNZ    COPY7                   ;if not full, get next record.
        JMPS   COPY9                   ;full, start first write session.
; indicate end-of-file read
COPY8:
        MOV    AL,TRUE                 ;set 'eof' flag
        MOV    Byte Ptr EOFLAG,AL
; write 'read-file' from memory buffer to destination 'written-file'
COPY9:
        MOV    AL,Byte Ptr R@U@A       ;set user to requested..
        CALL   SET@USR                 ;..area.
        MOV    BX,Word Ptr BUFSTART    ;adjust buffer pointer..
        MOV    Word Ptr BUF@PT,BX      ;..to start address.
COPY10:
        MOV    BX,Word Ptr REC@CNT     ;buffer empty?
        MOV    AL,BH
        OR     AL,BL
        JZ     COPY11                  ;buffer empty, check 'eof' flag.
        DEC    BX                      ;dec buffer record count for each write
        MOV    Word Ptr REC@CNT,BX
        MOV    BX,Word Ptr BUF@PT      ;set up dma address
        PUSH   BX                      ;save for size bump
        XCHG   BX,DX                   ;pointer in de-pair
        MOV    CL,SETDMA
        INT    224
        POP    BX
        MOV    DX,128                  ;bump pointer one record length
        ADD    BX,DX
        MOV    Word Ptr BUF@PT,BX
        MOV    DX,(Offset D@FCB)       ;destination file 'fcb'
        MOV    CL,WRITE                ;write record function
        INT    224
        OR     AL,AL                   ; 00h --> write okay
        JZ     COPY10                  ;okay, do next record.  else..
        CALL   ERMSG                   ;..say disk write error.
        DB     'Copy Disk Full',0
C@ERA:
        MOV    DX,(Offset D@FCB)       ;delete..
        MOV    CL,ERASE                ;..partial..
        INT    224                     ;..from directory.
        XOR    AL,AL                   ;reset 1st-time-thru tag flag..
        MOV    Byte Ptr FIRST@M,AL     ;..for continuation of mass copying.
        JMP    LOOPFN                  ;back to ring
COPY11:
        MOV    AL,Byte Ptr EOFLAG      ;buffer all written, check for 'eof'.
        OR     AL,AL
        JNZ    L_C19   
        JMP    COPY6A                  ;branch to read next buffer full
L_C19:
        MOV    DX,(Offset D@FCB)       ;point at 'fcb' for file closure
        MOV    CL,CLOSE
        INT    224
        INC    AL                      ;if no-close-error then..
        JNZ    CRC@CMP                 ;..compare file crc's.
        CALL   ERMSG
        DB     'Copy Close Error',0
        JMPS   C@ERA
; read destination 'written-file' and compare crc's
CRC@CMP:
        MOV    BX,Word Ptr CRCVAL      ;transfer 'crc' value to..
        MOV    Word Ptr CRCVAL2,BX     ;..new storage area.
        MOV    BX,0                    ;clear working storage..
        MOV    Word Ptr CRCVAL,BX      ;..to continue.
        MOV    DX,TBUF
        MOV    CL,SETDMA
        INT    224
        MOV    BX,(Offset D@FCB)+12
        CALL   INITFCB
        MOV    DX,(Offset D@FCB)
        MOV    CL,OPEN
        INT    224
        INC    AL                      ; 0ffh --> 00h if bad open
        JNZ    L_C20   
        JMP    BADCRC                  ;if bad open, just say 'bad-crc'.
L_C20:
        XOR    AL,AL                   ;zero 'fcb'..
        MOV    Byte Ptr D@FCB+32,AL    ;..'cr' field.
CRCWF1:
        MOV    DX,(Offset D@FCB)
        MOV    CL,READ
        INT    224
        OR     AL,AL                   ;read okay?
        JZ     D@RD@OK                 ;yes, read more.
        DEC    AL                      ;eof?
        JNZ    L_C21   
        JMP    FINCRC                  ;yes, finish up and make 'crc' comparison.
L_C21:
        CALL   ERMSG
        DB     'Copy Read Error',0
        JMP    NEUTRAL
D@RD@OK:
        MOV    BX,TBUF
        MOV    CH,128
CRCWF2:
        MOV    AL,M                    ;get character to..
        CALL   UPDCRC                  ;..add to 'crc' value. 
        INC    BX
        DEC    CH
        JNZ    CRCWF2
        JMPS   CRCWF1
 ; Clear READ-ONLY only. Search metadata supplies the original filename
; attribute bits, so a failed delete cannot clear SYSTEM/archive/F1-F3.
; HIDDEN is not a filename bit and is never translated into one here.
ATTRIB:
        PUSH BX
        MOV DX,Offset HB_DMA
        MOV CL,SETDMA
        INT 224
        POP DX
        PUSH DX
        MOV CL,SRCHF
        INT 224
        CMP AL,0FFH
        JE HB_ATTR_MISSING
        AND AX,3
        MOV CL,5
        SHL AX,CL
        MOV BX,Offset HB_DMA
        ADD BX,AX
        INC BX
        POP DX
        PUSH DX
        INC DX
        MOV CH,11
HB_ATTR_COPY:
        MOV AL,M
        MOV SI,DX
        MOV [SI],AL
        INC BX
        INC DX
        DEC CH
        JNZ HB_ATTR_COPY
        POP BX
        AND Byte Ptr 9[BX],7FH
        PUSH BX
        MOV CL,SETDMA
        MOV DX,TBUF
        INT 224
        POP DX
        MOV CL,ATTR
        INT 224
        RET
HB_ATTR_MISSING:
        POP BX
        MOV CL,SETDMA
        MOV DX,TBUF
        INT 224
        RET
; crc subroutines
; initialize tables for fast crc calculations
INITCRC:
        MOV    BX,(Offset CRCTBL)
        MOV    CL,0                    ;table index
GLOOP:
        XCHG   BX,DX
        MOV    BX,0                    ;initialize crc register pair
        MOV    AL,CL
        PUSH   CX                      ;save index in c-reg
        MOV    CH,8
        XOR    AL,BH
        MOV    BH,AL
LLOOP:
        SHL    BX,1
        JNB    LSKIP
        MOV    AL,10H                  ;generator is x^16 + x^12 + x^5 + x^0 as..
        XOR    AL,BH                   ;..recommended by ccitt for asynchronous..
        MOV    BH,AL                   ;..communications.  produces the same..
        MOV    AL,21H                  ;..results as public domain programs..
        XOR    AL,BL                   ;..chek, comm7, mdm7, and modem7.
        MOV    BL,AL
LSKIP:
        DEC    CH
        JNZ    LLOOP
        POP    CX
        XCHG   BX,DX                   ;de-pair now has crc, hl pointing into table.
        MOV    M,DH                    ;store high byte of crc..
        INC    BH
        MOV    M,DL                    ;..and store low byte.
        DEC    BH
        LAHF                           ;move to next table entry
        INC    BX
        SAHF
        INC    CL                      ;next index
        JNZ    GLOOP
        RET
UPDCRC:
        PUSH   CX                      ;update 'crc'..
        PUSH   BX                      ;..accumulator..
        MOV    BX,Word Ptr CRCVAL      ;pick up partial remainder
        XCHG   BX,DX                   ;de-pair now has partial
        MOV    CH,0
        XOR    AL,DH
        MOV    CL,AL
        MOV    BX,(Offset CRCTBL)
        ADD    BX,CX
        MOV    AL,M
        XOR    AL,DL
        MOV    DH,AL
        INC    BH
        MOV    DL,M
        XCHG   BX,DX
        MOV    Word Ptr CRCVAL,BX
        POP    BX
        POP    CX
        RET
FINCRC:
        MOV    AL,Byte Ptr C@U@A       ;reset user from 'requested'..
        CALL   SET@USR                 ;..to 'current' area.
        MOV    BX,Word Ptr CRCVAL      ;put written-file 'crc' into..
        XCHG   BX,DX                   ;..de-pair.
        MOV    BX,Word Ptr CRCVAL2     ;put read-file 'crc' and..
        CALL   CMPDEHL                 ;..compare 'de/hl' for equality.
        JNZ    BADCRC                  ;if not zero, show copy-error message.
        CALL   ILPRT                   ;if zero, show 'verified' message.
        DB     ' -- CRC Verified',0
        MOV    AL,Byte Ptr MFLAG       ;if not mass-copy mode, return..
        OR     AL,AL                   ;..to next 'ring' position.
        JZ     L_C22   
        JMP    FORWARD                 ;else..
L_C22:
        NOT    AL                      ;..set 1st-time-thru flag..
        MOV    Byte Ptr FIRST@M,AL     ;..and..
        JMP    M@LP                    ;..get next file to copy, if one.
BADCRC:
        CALL   ERMSG
        DB     'Error on CRC compare',0
        JMP    FORWARD                 ;move to next 'ring' position
