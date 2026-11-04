L_C36  EQU     $
        DSEG
        ORG    Offset L_C36

;Intialized Storage
; Persistent browse options and metadata, outside the movable list.
HB_ALL DB 0
HB_FAT DB 0
HB_LAST DB 0
HB_DRIVE DB 0
HB_RAW DB '????????????'
       DB 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
HB_DMA RS 128 ; private BDOS17 DMA for non-destructive RO override

CHR_LO      DB  0                       ; CHR_LO    *added -km
CHR_HI      DB  0                       ; CHR_HI    *added -km

; VT52 sequence letter -> the scan code FTABLE was written against
ESCTAB  DB     'A'
        DB     ARROW@UP
        DB     'B'
        DB     ARROW@DOWN
        DB     'C'
        DB     ARROW@RIGHT
        DB     'D'
        DB     ARROW@LEFT
        DB     'H'
        DB     HOME@KEY
        DB     'I'
        DB     PG@UP
        DB     0                        ;end of table

CLIERR         DB      0
CLIUSER        DB      0
CLIDRIVE       DB      0
CLIPRIO        DB      0
CLICB          DB      0
               RS      129
CCKEYS         DB      0                ;concurrent keyboard backend
HELPFLG        DB      0                ;is HELP available externally?  0=No

HEADMSG        DB      'File: ',0

DATE   DB      '      '

;-------------------------------------------------------------------
 IF     CLOCK
        DATESTR DB  'MM/DD/YY  MM:MM:SS' ;date & time go here
 ENDIF
        DB     0

MOREHELP RS     0
        DB     'A'                     ;SET BY HELPCHK ROUTINE
        DB     ':HELP FILER',0        ;HELP Command for further info
HELPFCB RS      0
        DB      0,'HELP    CMD'
FILERCMD RS     0
        DB     '!FILER '
FILE@D  RS      0
        DB     'x'
FILE@U  RS      0
        DB     'xx'
        DB     ' W'                    ;WAIT option
        DB     0
JOKER  RS      0
        DB     '???????????'           ; *.* equivalent
FIRST@M RS     0
        DB     FALSE                   ; 1st time thru in mass-copy mode
MFLAG   RS     0
        DB     TRUE                    ;multiple file copy flag --> 0 for mass copy
TAG@TOT RS     0
        DW     0                       ;summation of tagged file sizes
CMDBUF  RS     0
        DB     32,0                    ;command buffer maximum length, usage, and..

; uninitialized data
        RS     32                      ;..storage for the command buffer.
        RS     256                     ;local stack, deepest path ~50 bytes + interrupts
STACK   RS     0
        RS     2                       ;cp/m's stack pointer stored here
B@MAX   RS     0
        RS     2                       ;highest block number on drive
B@MASK  RS     0
        RS     1                       ;sec/blk - 1
BSHIFTF RS     0
        RS     1                       ; # of shifts to multiply by sec/blk
BUF@PT  RS     0
        RS     2                       ;copy buffer current pointer..
BUFSTART RS    0
        RS     2                       ;..and begin pointer.
CANFLG  RS     0
        RS     1                       ;no-file-found cancel flag
C@DR    RS     0
        RS     1                       ; 'current drive'
CHARCNT RS     0
        RS     1                       ;character count for tab expansion
CON@LST RS     0
        RS     1                       ;bdos function storage
CRCTBL  RS     0
        RS     512                     ;tables for 'crc' calculations
CRCVAL  RS     0
        RS     2                       ; 2-byte 'crc' value of working file and..
CRCVAL2 RS     0
        RS     2                       ;..of finished source read-file.
C@U@A   RS     0
        RS     1                       ; 'current user area'
CURAT   RS     0
        RS     2                       ;current cursor position
D@FCB   RS     0
        RS     33                      ;fcb for destination file/new name if rename
DIRNAME RS     0
        RS     2                       ;ptr to DIR prefix
DISK    RS     0
        RS     1                       ;selected disk for ZDNAME
DISKSP  RS     0
        RS     2                       ;space remaining on disk
DNLOAD  RS     0
        RS     1                       ;NAMES.DIR loaded flag
DRLET   RS     0
        RS     1                       ;scratch for drive letter
ENTRY   RS     0
        RS     11                      ;scratch for ZDNAME/ZDNFIND
EOFLAG  RS     0
        RS     1                       ;file copy loop 'eof' flag
ERMFLG  RS     0
        RS     1                       ;error message present flag
FICHAR  RS     0
        RS     1                       ;byte-oriented input char
FIPTR   RS     0
        RS     2                       ;byte-oriented input ptr
FSDFLG  RS     0
        RS     1                       ;display file size flag (yes/no)
FS@FLG  RS     0
        RS     1                       ;tag total versus file size flag
LDSP    RS     0
        RS     1                       ;leading space count for DECOUT
LPSCNT  RS     0
        RS     1                       ;lines-per-screen for 'view'
LOCBEG  RS     0
        RS     2                       ;local beginning of ring
LOCEND  RS     0
        RS     2                       ;local end of ring
LOCPOS  RS     0
        RS     2                       ;local ring position (temp)
MAXDR   RS     0
        RS     1                       ;max driver letter
MDFLG   RS     0
        RS     1                       ;mass delete verify flag
O@USR   RS     0
        RS     1                       ;store initial user area for exit
R@DR    RS     0
        RS     1                       ; 'requested drive'
RCNT    RS     0
        RS     2                       ; # of records in file and..
REC@CNT RS     0
        RS     2                       ;..currently in ram buffer.
REC@MAX RS     0
        RS     2                       ;maximum 128-byte record capacity of buffer
RING    RS     0
        RS     2                       ;ptr to beginning of ring
RINGI   RS     0
        RS     2                       ;ring sort pointer
RINGJ   RS     0
        RS     2                       ;another ring sort pointer
RINGEND RS     0
        RS     2                       ;current ring end pointer
RINGPOS RS     0
        RS     2                       ;current ring position in scan
R@U@A   RS     0
        RS     1                       ; 'requested user area'
SCURAT  RS     0
        RS     2                       ;save cursor position
S@FCB   RS     0
        RS     36                      ;fcb for source (random record) file
SRINGPOS RS    0
        RS     2                       ;save ring position
T@DR    RS     0
        RS     1                       ;temp disk
TEST@RT RS     0
        RS     1                       ;intermediate right-justify data
T@U@A   RS     0
        RS     1                       ;temp user
T@UN@FG RS     0
        RS     1                       ;tag/untag file summation switch
USER    RS     0
        RS     1                       ;temp user buffer
VIEWFLG RS     0
        RS     1                       ; 00h --> to list/punch else to crt 'view'
Z@DR    RS     0
        RS     1                       ;disk for ZDNAME
Z@U@A   RS     0
        RS     1                       ;user area for ZDNAME
DATA_SEG RS    2                       ;program data segment
PAGEFLG RS     1                       ;true for printing pages
HEADFLG RS     1                       ;true for printing header
LTPP1   RS     1                       ;modified lines per page
MCFLG   RS     1                       ;mass copy verify flag



; assembled 'com' and 'ram-loaded' file size (0c00h = 3k)
COMFILE        EQU     ((Offset CMDBUF)+2)-256 ; 'prn' listing shows 'com'..
ALAST  RS      1                       ; 1 for byte before BUFENTRY
; even-page base of filename ring storage
BUFENTRY EQU   (((Offset $)/100H)*100H)+100H
