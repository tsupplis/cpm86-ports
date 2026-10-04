;*-------------------------------------------------------------------------
;  EDLIN.ASM - IBM Personal Computer line editor ("EDITOR"), version 1.00
;
;  Build:   make edlin.com
;           (msmasm edlin -> link edlin -> exe2bin edlin.exe edlin.com)
;
;  Usage:   EDLIN [d:]filename
;           Edits filename; the previous version is kept as filename.BAK.
;           Edits go to a temporary filename.$$$ that replaces the file on E.
;
;  Commands (n, m = line number; "." = current line, "#" = last line):
;     [n]              Edit line n (default: the line after the current one)
;     [n][,m]L         List lines (default: 23 lines around the current line)
;     [n][,m]D         Delete lines
;     [n]I             Insert lines before line n (end with ^Z or ^Break)
;     [n][,m][?]Sstr   Search for str, ? asks "O.K.?" for each match
;     [n][,m][?]Rstr^Zrepl   Replace str by repl, ? asks "O.K.?"
;     [n]A             Append n lines from the file (fill memory by default)
;     [n]W             Write n lines to disk (frees memory for A)
;     E                End edit: save, rename old file to .BAK
;     Q                Quit edit without saving (asks "Abort edit (Y/N)?")
;
;  Memory (COM image, single segment, org 100h):
;     0000h-00FFh   PSP (FCB1 at 5Ch holds the file given on the command line)
;     0100h-093Dh   code
;     093Eh-0A57h   message strings (ends the COM image)
;     0A58h-0D48h   uninitialised work area: FCB2, variables, line buffers,
;                   and the stack (see the equates at the end of the file)
;     0D49h-top     text buffer: the lines of the file, each ending with
;                   CR LF, followed by a ^Z (stack_top, just before the buffer,
;                   holds a LF so every line is preceded by one)
;
;  Routines take their arguments in registers, as noted at each one.  The
;  Z flag is often the result code (set = found / yes).
;
;  Labels ending in _ret (skip_ret, scan_eof_ret, write_recs_ret, ...) sit on
;  the RET that ends the routine they are named after.  Other routines branch
;  to them instead of carrying a RET of their own, as the original does.
;-------------------------------------------------------------------------

program         segment
                assume  cs:program, ds:program
                org     100h

; ---------------------------------------------------------------------------
; Macros for encodings the original program has and MASM 1.10 does not emit
;   jmpn:      near jmp (E9) to a label that is in short range; MASM 1.10 would
;              use a short jmp (EB) for a backward target and `near ptr` is
;              ignored
;   cmp82_*:   cmp r/m8,imm8 with opcode 82h, which MASM 1.10 encodes as 80h
; ---------------------------------------------------------------------------
jmpn            macro   target
                db      0E9h
                dw      target - $ - 2
                endm

cmp82_mem       macro   addr, imm               ; cmp byte ptr ds:[addr], imm
                db      82h, 3Eh
                dw      addr
                db      imm
                endm

cmp82_si        macro   imm                     ; cmp byte ptr [si], imm
                db      82h, 3Ch, imm
                endm

cmp82_cl        macro   imm                     ; cmp cl, imm
                db      82h, 0F9h, imm
                endm

; ---------------------------------------------------------------------------
; Character constants
; ---------------------------------------------------------------------------
cr              equ     0Dh             ; Carriage return
lf              equ     0Ah             ; Line feed
ctrlz           equ     1Ah             ; ^Z, end of file / end of text marker
tab             equ     09h             ; Tab
upmask          equ     5Fh             ; AND mask: lower case letter to upper case
ctrlmask        equ     40h             ; OR mask: control character to its letter

; ---------------------------------------------------------------------------
; FCB layout
; ---------------------------------------------------------------------------
fcb_newname     equ     16              ; Offset of the new name in a rename FCB
fcb_drvname     equ     9               ; Drive byte + 8 name characters
fcb_ext_len     equ     3               ; Extension length ("BAK", "$$$")
fcb_fname_words equ     6               ; Drive + name + extension: 12 bytes

; ---------------------------------------------------------------------------
; Program Segment Prefix (PSP) fields.  These are below the 100h load
; address, in the same segment as the code and data (COM program: CS=DS=ES=SS)
; ---------------------------------------------------------------------------
; CPM86 PORT: psp_memsize (PSP:06h) is NOT available memory under CP/M-86;
; CP/M-86 stores the code group length there, not the TPA ceiling.
; Use seg_max (compile-time equate) + CMD header -m 10000 instead.
;psp_memsize     equ     06h             ; NOT USED under CP/M-86 -- see seg_max
fcb1            equ     5Ch             ; Default FCB 1 (file named on the command line)
fcb1_name       equ     fcb1 + 1        ; File name (8 chars, space padded)
fcb1_ext        equ     fcb1 + 9        ; Extension (3 chars)
fcb1_recsiz     equ     fcb1 + 14       ; Record size word
fcb1_newname    equ     fcb1 + fcb_newname ; Rename: new name field (for reference: the E command
                                        ; reaches it as [si + fcb_newname] with SI = fcb1)
fcb1_rr         equ     fcb1 + 33       ; Random record number (dword)

; ---------------------------------------------------------------------------
; CPM86 PORT: DOS INT 21h/20h -> CP/M-86 BDOS INT 0E0h, CL=function
; Function numbers 01h-1Ah are identical.
; ---------------------------------------------------------------------------
bdos_kbd_echo   equ     01h             ; Console input with echo
bdos_display    equ     02h             ; Console output (DL=char)
bdos_print      equ     09h             ; Print '$' terminated string (DX=addr)
bdos_bufin      equ     0Ah             ; Buffered console input (DX=buf)
bdos_open       equ     0Fh             ; Open file (DX=FCB)
bdos_close      equ     10h             ; Close file
bdos_delete     equ     13h             ; Delete file
bdos_create     equ     16h             ; Create file
bdos_rename     equ     17h             ; Rename file
bdos_set_dta    equ     1Ah             ; Set DMA address offset (DX=offset)
bdos_dma_seg    equ     33h             ; Set DMA segment (DX=segment)
; CPM86 PORT ST4: DOS fn 27h/28h replaced by CP/M-86 fn 21h/22h (F_READRAND/F_WRITERAND)
; DOS fn 27h = random block read (by count), fn 28h = random block write -- NOT in CP/M-86
; CP/M-86 fn 21h = F_READRAND (1 record at fcb_rr), fn 22h = F_WRITERAND (same)
; read_block / write_block procs implement the loop; see near print_crlf
bdos_rdrand     equ     21h             ; F_READRAND:  read 1 record at FCB random counter
bdos_wrrand     equ     22h             ; F_WRITERAND: write 1 record at FCB random counter
; CPM86 PORT: dos_setvec_23 removed -- no CP/M-86 equivalent for INT 21h AH=25h
;dos_setvec_23  equ     2523h           ; AH = 25h set interrupt vector, AL = 23h (^C)

; ---------------------------------------------------------------------------
; Limits and special values
; ---------------------------------------------------------------------------
seg_max         equ     0FFFFh          ; Full 64 KB segment -- booked in CMD header via -m 10000
last_line       equ     0FFFEh          ; Line number returned for "#"
all_lines       equ     0FFFFh          ; "Every line" count
numlim          equ     1999h           ; 6553 = 65535 / 10: next digit would overflow
combuf_size     equ     128                     ; Command line max length
strbuf_size     equ     128                     ; Search / replace string max length
editbuf_max     equ     255                     ; Edited line max length
stack_size      equ     40                      ; Bytes reserved for the stack
max_linelen     equ     254             ; Longest line allowed after a replace
list_before     equ     11              ; List: lines shown before the current line
list_count      equ     23              ; List: default number of lines shown

_start:
                ; CPM86 PORT ST3: DS=base-page segment on entry; point DS and ES at CS
                mov     ax, cs
                mov     ds, ax
                mov     es, ax
                jmp     short init
; ---------------------------------------------------------------------------
; Copyright banner (never displayed by the program, '$' terminated)
; ---------------------------------------------------------------------------
                db      cr, lf
                db      "The IBM Personal Computer EDITOR", cr, lf
                db      "Version 1.00 (C)Copyright IBM Corp 1981", cr, lf, "$"
                db      "Licensed Material - Program Property of IBM"

; ---------------------------------------------------------------------------
; Startup: validate the file name, open it, create the temporary file
; ---------------------------------------------------------------------------

err_nofile:
                mov     dx, offset nofile

err_exit:
                jmp     disp_err

init:
                ; CPM86 PORT ST3: set DMA segment to CS so file I/O lands in our segment
                mov     cl, bdos_dma_seg        ; BDOS fn 33h = Set DMA Segment
                mov     dx, cs
                int     0E0h
                mov     byte ptr ds:[modflg], 0 ; Not in "end edit" mode
                mov     sp, offset stack_top
                cmp82_mem fcb1_name, ' '        ; No file name on the command line?
                jz      short err_nofile        ; No file name on the command line
                or      al, al                  ; AL = 0 if the drive letter is valid
                mov     dx, offset baddrv
                jnz     short err_exit
                mov     si, offset bak          ; Refuse to edit a .BAK file
                mov     di, fcb1_ext
                mov     cx, fcb_ext_len
                repe cmpsb
                jz      short err_bak
                ; CPM86 PORT: int 21h AH= -> int 0E0h CL=
                mov     cl, bdos_open                 ; DOS: open file (FCB)
                mov     dx, fcb1
                int     0E0h            ; CPM86 PORT: int 21h -> int 0E0h
                mov     byte ptr ds:[newfile_flg], al ; 0 = file found, 0FFh = new file
                or      al, al
                jz      short create_tmp
                mov     dx, offset newfil
                ; CPM86 PORT: int 21h AH= -> int 0E0h CL=
                mov     cl, bdos_print                  ; BDOS: print string
                int     0E0h                    ; "New file"

create_tmp:
                mov     si, fcb1                ; Copy drive + 8 char name to FCB 2
                mov     di, offset fcb2
                mov     cx, fcb_drvname
                rep movsb
                mov     si, offset bak          ; ... and give it the extension BAK
                movsw
                movsb
                ; CPM86 PORT: int 21h AH= -> int 0E0h CL=
                mov     cl, bdos_delete                ; BDOS: delete file (FCB), any old .BAK
                mov     dx, offset fcb2
                int     0E0h
                mov     al, '$'
                mov     di, offset fcb2_ext     ; Extension becomes "$$$"
                stosb
                stosb
                stosb
                ; CPM86 PORT: int 21h AH= -> int 0E0h CL=
                mov     cl, bdos_create                ; BDOS: create the temporary file
                int     0E0h
                or      al, al
                jz      short init_buf
                mov     dx, offset nodir        ; Directory full
                jmp     disp_err

err_bak:
                mov     dx, offset nobak
                jmp     disp_err

; ---------------------------------------------------------------------------
; Buffer setup and initial file read
; ---------------------------------------------------------------------------

init_buf:
                xor     ax, ax
                mov     ds:[fcb1_rr], ax        ; Random record 0 in both FCBs
                mov     ds:[fcb1_rr + 2], ax
                mov     word ptr ds:[fcb2_rr], ax
                mov     word ptr ds:[fcb2_rr + 2], ax
                mov     ax, 128                 ; CPM86 PORT ST4: 128-byte records (CP/M-86 standard)
                mov     ds:[fcb1_recsiz], ax
                mov     word ptr ds:[fcb2_recsiz], ax
                mov     dx, offset buf_start
                mov     di, dx

                ; CPM86 PORT: int 21h AH= -> int 0E0h CL=; push DI (BDOS clobbers, DI=buf_start used at add di,cx)
                push    di
                mov     cl, bdos_set_dta                ; BDOS: set DMA offset to text buffer
                int     0E0h
                pop     di
                ; CPM86 PORT ST3: use seg_max (compile-time 64 KB) instead of psp_memsize
                mov     cx, seg_max             ; Full 64 KB segment booked in CMD header
                mov     word ptr ds:[mem_top], cx ; Last usable address of the buffer
                test    byte ptr ds:[newfile_flg], 0FFh
                jnz     short init_vars         ; New file: nothing to read
                sub     cx, offset buf_start    ; Buffer capacity in bytes
                shr     cx, 1                   ; 1/2 of the buffer
                mov     ax, cx
                shr     cx, 1
                mov     word ptr ds:[buf_1qtr], cx ; 1/4 of the buffer (size)
                add     cx, ax                  ; 3/4 of the buffer (size)
                mov     dx, cx
                add     dx, offset buf_start
                mov     word ptr ds:[buf_3qtr], dx ; 3/4 mark (address)
                ; CPM86 PORT ST4: convert byte count to record count, call read_block
                add     cx, 127
                push    cx
                mov     cl, 7
                pop     ax
                shr     ax, cl                  ; AX = ceil(bytes/128) = record count
                mov     dx, fcb1
                call    read_block              ; AX=recs, DX=fcb1, DI=buf_start -> CX=bytes placed
                call    scan_eof                ; CX = bytes up to ^Z, if any
                add     di, cx                  ; DI = end of the text read

init_vars:
                cld
                mov     byte ptr [di], ctrlz    ; Terminate the text
                mov     word ptr ds:[endtxt], di
                mov     byte ptr ds:[combuf], combuf_size ; Max length for command line input
                mov     byte ptr ds:[editbuf], editbuf_max ; Max length for line editing
                mov     byte ptr ds:[stack_top], lf ; Sentinel LF just before the text buffer
                mov     word ptr ds:[pointer], offset buf_start ; Current line = first line
                mov     word ptr ds:[curlin], 1
                mov     word ptr ds:[param1], 1 ; Default argument for the first append
                test    byte ptr ds:[newfile_flg], 0FFh
                jnz     short command           ; New file: start with an empty buffer
                call    append_cmd              ; Read the rest of the file into memory

; ---------------------------------------------------------------------------
; Main command loop: prompt, read a command line, parse "[n][,m][?]c" and
; dispatch on the command letter c.  Every command returns to "command".
; ---------------------------------------------------------------------------

command:
                mov     sp, offset stack_top    ; Discard anything left on the stack
                ; CPM86 PORT: ^C vector install removed -- no equivalent; ^C triggers warm boot
                mov     al, '*'                 ; Prompt
                call    print_char
                mov     dx, offset combuf       ; DOS: buffered keyboard input
                ; CPM86 PORT: int 21h AH= -> int 0E0h CL=
                mov     cl, bdos_bufin
                int     0E0h
                mov     al, lf                  ; The input only echoed a CR
                call    print_char
                mov     word ptr ds:[param2], 0
                mov     byte ptr ds:[qflg], 0
                mov     si, offset combuf + 2   ; Typed text follows the 2 length bytes
                call    getnum                  ; First number
                mov     word ptr ds:[param1], dx
                call    skip1                   ; AL = next non blank character
                cmp     al, ','
                jnz     short chknxt
                inc     si                      ; Step over the comma (undone below)

chknxt:
                dec     si                      ; Re-read the character after the number
                call    getnum                  ; Second number
                mov     word ptr ds:[param2], dx
                call    skip1
                cmp     al, '?'                 ; Query flag, ask before each change
                jnz     short chkcase
                mov     byte ptr ds:[qflg], al
                call    skip                    ; Command letter follows the "?"

chkcase:
                cmp     al, '_'                 ; Convert lower case letters to upper
                jbe     short dispatch
                and     al, upmask

dispatch:
                mov     di, offset comtab
                mov     cx, offset comtab_end - offset comtab ; Number of entries
                repne scasb
                jnz     short comerr            ; Unknown command letter
                mov     bx, cx                  ; CX = table length - 1 - index of the
                                                ; match, which is the index into "table"
                mov     ax, word ptr ds:[param2]
                or      ax, ax
                jz      short docom
                cmp     ax, word ptr ds:[param1]
                jb      short comerr            ; Second number is below the first

docom:
                shl     bx, 1                   ; Word index
                call    table[bx]
                jmpn    command                 ; Back to the command prompt

; ---------------------------------------------------------------------------
; Skip blanks in the command line.
; skip:  read the next character first
; skip1: start with the character already in AL
; Returns: AL = first non blank character, SI = address after it
; ---------------------------------------------------------------------------

skip            proc near
                lodsb

skip1:
                cmp     al, ' '
                jz      short skip

skip_ret:
                ret
skip            endp

; ---------------------------------------------------------------------------
; Command error: display "Entry error" and return to the prompt
; ---------------------------------------------------------------------------

comerr:
                mov     dx, offset badcom
                ; CPM86 PORT: int 21h AH= -> int 0E0h CL=
                mov     cl, bdos_print                  ; BDOS: print string
                int     0E0h
                jmp     command

; ---------------------------------------------------------------------------
; Parse a line number from the command line at SI.
; Accepts decimal digits, "." (current line) and "#" (last line).
; Returns: DX = line number (0 if none), AL = character after the number,
;          SI = address after that character
; An invalid number (too big, or 0 typed explicitly) is a command error.
; ---------------------------------------------------------------------------

getnum          proc near
                call    skip
                cmp     al, '.'
                jz      short curlin_param
                cmp     al, '#'
                jz      short maxlin_param
                mov     dx, 0
                mov     cl, 0                   ; CL = 1 once a digit was seen

numlp:
                cmp     al, '0'
                jb      short chknum
                cmp     al, '9'
                ja      short chknum
                cmp     dx, numlim              ; Another digit would overflow 16 bits
                jnb     short comerr
                mov     cl, 1
                sub     al, '0'
                mov     bx, dx
                shl     dx, 1
                shl     dx, 1
                add     dx, bx                  ; DX * 5
                shl     dx, 1                   ; DX * 10
                cbw                             ; AX = digit
                add     dx, ax
                lodsb
                jmp     short numlp

chknum:
                cmp82_cl 0                      ; Any digit seen?
                jz      short skip_ret            ; No number given, DX = 0
                or      dx, dx                  ; An explicit 0 is invalid
                jz      short comerr
                ret

curlin_param:
                mov     dx, word ptr ds:[curlin] ; "." is the current line
                lodsb
                ret

maxlin_param:
                mov     dx, last_line           ; "#" is past any real line
                lodsb
                ret
getnum          endp

; ---------------------------------------------------------------------------
; Command table.  "comtab" holds the command letters, with CR standing for
; "no command".  REPNE SCASB leaves CX = length - 1 - index of the match, so
; "table" lists the handlers in reverse order of the letters.
; ---------------------------------------------------------------------------

comtab          db      "QWASRDLIE", cr
comtab_end      label   byte

table           dw      offset edit_cmd         ; CR  Edit a line
                dw      offset exit_cmd         ; E   End edit, save the file
                dw      offset insert_cmd       ; I   Insert lines
                dw      offset list_cmd         ; L   List lines
                dw      offset delete_cmd       ; D   Delete lines
                dw      offset replace_cmd      ; R   Replace text
                dw      offset search_cmd       ; S   Search for text
                dw      offset append_cmd       ; A   Append lines from the file
                dw      offset write_cmd        ; W   Write lines to the file
                dw      offset quit_cmd         ; Q   Quit without saving

; ---------------------------------------------------------------------------
; Look for a ^Z in CX bytes at ES:DI.
; Returns: ZF set if a ^Z was found, CX = number of bytes scanned (up to and
;          including the ^Z), DI unchanged
; ---------------------------------------------------------------------------

scan_eof        proc near
                push    di
                push    cx
                mov     al, ctrlz
                repne scasb
                mov     di, cx                  ; Bytes not scanned
                pop     cx
                lahf                            ; Keep ZF across the subtraction
                sub     cx, di
                sahf
                pop     di

scan_eof_ret:
                ret
scan_eof        endp

; ---------------------------------------------------------------------------

append_eof_exit:
                jmp     append_eof_msg

; ---------------------------------------------------------------------------
; A command: [n]A
; Read more of the file into memory after the current text.  With n, read
; exactly n lines; without, read until memory is 3/4 full (cut on a line end).
; "newfile_flg" tells whether the whole file has been read already.
; Also called by init and by the E command.
; ---------------------------------------------------------------------------

append_cmd      proc near
                test    byte ptr ds:[newfile_flg], 0FFh
                jnz     short append_eof_exit   ; Nothing left to read
                mov     dx, word ptr ds:[endtxt]
                cmp     word ptr ds:[param1], 0
                jnz     short append_lp
                cmp     dx, word ptr ds:[buf_3qtr]
                jnb     short scan_eof_ret      ; Already 3/4 full, nothing to do

append_lp:
                mov     di, dx                  ; DI = where the new text starts
                ; CPM86 PORT: AH= -> CL=; push DI+DX (BDOS clobbers both; DX used in sub cx,dx below)
                push    di
                push    dx
                mov     cl, bdos_set_dta        ; BDOS: set DMA offset to end of text
                int     0E0h
                pop     dx
                pop     di
                mov     cx, word ptr ds:[mem_top]
                sub     cx, dx                  ; Free memory
                jnz     short append_lp_noerr   ; ST4: append_memerr now out of short range
                jmp     append_memerr
append_lp_noerr:
                ; CPM86 PORT ST4: convert byte count to record count, call read_block
                add     cx, 127
                push    cx
                mov     cl, 7
                pop     ax
                shr     ax, cl                  ; AX = ceil(bytes/128) = record count
                mov     dx, fcb1
                call    read_block              ; AX=recs, DX=fcb1, DI=endtxt -> CX=bytes placed, AL=status
                mov     byte ptr ds:[newfile_flg], al ; Non zero when the end of the file was hit
                push    cx                      ; Bytes effectively read (records * 128)
                call    scan_eof
                jnz     short append_check_limit
                mov     byte ptr ds:[newfile_flg], 1 ; ^Z found: that is the end of the file

append_check_limit:
                xor     dx, dx                  ; Lines counted so far
                mov     bx, word ptr ds:[param1] ; Lines wanted
                or      bx, bx
                jnz     short append_find_lines
                mov     ax, di                  ; No count: keep at most up to the
                add     ax, cx                  ; 3/4 mark, plus the rest of its line
                cmp     ax, word ptr ds:[buf_3qtr]
                jbe     short append_find_lines
                mov     di, word ptr ds:[buf_3qtr]
                mov     cx, ax
                sub     cx, di                  ; Bytes beyond the mark
                mov     bx, 1                   ; ... up to the end of one line

append_find_lines:
                call    count_lines             ; DI = after BX lines (or at the end)
                cmp     [di-1], al              ; AL = LF: text ends on a line end?
                jz      short append_set_end
                std                             ; No: back up to the previous line end
                dec     di
                mov     cx, word ptr ds:[mem_top]
                repne scasb
                inc     di
                inc     di
                dec     dx                      ; The partial line does not count
                cld

append_set_end:
                pop     cx                      ; Bytes read
                mov     word ptr [di], ctrlz    ; New end of the text
                sub     cx, di
                xchg    di, word ptr ds:[endtxt]
                add     di, cx                  ; DI = unused bytes (16-bit wrap arithmetic)
                ; CPM86 PORT ST4: fcb1_rr counts 128-byte records; convert unused bytes to records
                mov     ax, di
                add     ax, 127
                mov     cl, 7
                shr     ax, cl                  ; AX = ceil(unused_bytes/128) = unused records
                sub     ds:[fcb1_rr], ax        ; Move file position back
                sbb     word ptr ds:[fcb1_rr + 2], 0
                cmp     bx, dx
                jnz     short append_test_new   ; Fewer lines than wanted
                mov     byte ptr ds:[newfile_flg], 0 ; Got all the lines wanted
                ret

append_eof_msg:
                mov     dx, offset eofmsg       ; "End of input file"
                ; CPM86 PORT: AH= -> CL=
                mov     cl, bdos_print                   ; DOS: print string
                int     0E0h            ; CPM86 PORT: int 21h -> int 0E0h

append_ret:
                ret

append_test_new:
                test    byte ptr ds:[newfile_flg], 0FFh
                jnz     short append_eof_msg    ; The file ended before n lines
                test    byte ptr ds:[modflg], 0FFh
                jnz     short append_ret        ; During E, running out of room is fine

append_memerr:
                jmp     memerr
append_cmd      endp

; ---------------------------------------------------------------------------
; W command: [n]W
; Write the first n lines to the temporary file and move the rest of the text
; down.  Without n, write everything except the last 1/4 of a buffer (if the
; text is larger than that), cut on a line end (checked with emu2: a 46400 byte
; text left 15456 bytes in memory, 1/4 of the 61.9K buffer).
; ---------------------------------------------------------------------------

write_cmd       proc near
                mov     bx, word ptr ds:[param1]
                or      bx, bx
                jnz     short write_recs
                mov     cx, word ptr ds:[buf_1qtr]
                mov     di, word ptr ds:[endtxt]
                sub     di, cx                  ; Keep the last 1/4 buffer in memory
                jbe     short append_ret
                cmp     di, offset buf_start
                jbe     short append_ret        ; Not enough text to write
                xor     dx, dx
                mov     bx, 1
                call    count_lines             ; Round up to the end of that line
                jmp     short write_calc_len
write_cmd       endp

; ---------------------------------------------------------------------------
; Write the text before a given line to the temporary file, then move the
; rest of the text down to the start of the buffer.
; write_recs:       BX = number of lines to write (all_lines = everything)
; write_calc_len:   DI = address of the end of the text to write
; The new current line is line 1.  A write error ends the program.
; ---------------------------------------------------------------------------

write_recs      proc near
                inc     bx                      ; Find the line after the last one
                call    findlin                 ; to write (BX = 0 means up to the end)

write_calc_len:
                mov     cx, di
                mov     dx, offset buf_start
                sub     cx, dx                  ; CX = bytes to write
                jz      short append_ret
                ; CPM86 PORT ST4: convert byte count to record count, call write_block
                push    di                      ; save write-end addr (used as si below)
                add     cx, 127
                push    cx
                mov     cl, 7
                pop     ax
                shr     ax, cl                  ; AX = ceil(bytes/128) = record count
                mov     di, offset buf_start    ; DMA start = beginning of text
                mov     dx, offset fcb2
                call    write_block             ; AX=recs, DX=fcb2, DI=buf_start -> AL=status
                or      al, al
                jnz     short write_dskful_err
                pop     si                      ; si = write-end addr
                mov     di, offset buf_start    ; down to the start of the buffer
                mov     word ptr ds:[pointer], di
                mov     cx, word ptr ds:[endtxt]
                sub     cx, si
                inc     cx
                rep movsb
                dec     di
                mov     word ptr ds:[endtxt], di
                mov     word ptr ds:[curlin], 1

write_recs_ret:
                ret

write_dskful_err:
                ; CPM86 PORT: AH= -> CL=
                mov     cl, bdos_close          ; BDOS: close the temporary file
                int     0E0h
                mov     dx, offset dskful

; Display the error message and exit (no return)
disp_err:
                ; CPM86 PORT: AH= -> CL=
                mov     cl, bdos_print          ; BDOS: print string
                int     0E0h
                ; CPM86 PORT: int 20h -> BDOS fn 0 (warm boot / return to CCP)
                xor     cx, cx
                mov     dl, 0
                int     0E0h
write_recs      endp

; ---------------------------------------------------------------------------
; Find the address of a line.
; Takes:   BX = line number (0 = after the last line)
; Returns: DX = line number reached, DI = address of that line,
;          ZF set if line BX exists (the empty line just past the last line
;          counts), clear if past the end of the text
; The search starts from the current line when BX is after it, otherwise
; from the start of the buffer.  Falls into count_lines.
; ---------------------------------------------------------------------------

findlin         proc near
                mov     dx, word ptr ds:[curlin]
                mov     di, word ptr ds:[pointer]
                cmp     bx, dx
                jz      short write_recs_ret       ; It is the current line
                ja      short findlin_calc_rem  ; After it: search forward
                or      bx, bx
                jz      short findlin_calc_rem  ; 0 = to the end: also forward
                mov     dx, 1                   ; Before it: restart at line 1
                mov     di, offset buf_start
                cmp     bx, dx
                jz      short write_recs_ret

findlin_calc_rem:
                mov     cx, word ptr ds:[endtxt]
                sub     cx, di                  ; CX = text left to scan
findlin         endp

; ---------------------------------------------------------------------------
; Advance over lines.
; Takes:   DI = address of line DX, CX = bytes left in the text, BX = target
;          line number
; Returns: ZF set, DI = address of line BX, DX = BX; or ZF clear if the text
;          ran out first (DI = end of text)
; ---------------------------------------------------------------------------

count_lines     proc near
                mov     al, lf
                or      al, al                  ; ZF = 0 in case CX = 0

count_lp:
                jcxz    short write_recs_ret
                repne scasb                     ; Next LF
                inc     dx
                cmp     bx, dx
                jnz     short count_lp

count_lines_ret:
                ret
count_lines     endp

; ---------------------------------------------------------------------------
; Display a line number prompt: " " + 5 digit number + ":" + "*" (or " ")
; The "*" marks the current line.
; Takes:   BX = line number
; ---------------------------------------------------------------------------

shownum         proc near
                push    bx
                mov     al, ' '
                call    print_char
                call    conv10
                mov     al, ':'
                call    print_char
                mov     al, '*'
                pop     bx
                cmp     bx, word ptr ds:[curlin]
                jz      short shownum_space
                mov     al, ' '

shownum_space:
                jmp     print_char
shownum         endp

; ---------------------------------------------------------------------------
; Display BX as a 5 digit decimal number, leading zeros as blanks.
; The binary to BCD conversion shifts BX left 16 times, doubling the BCD
; result held in DL:AH:AL each time and adding the bit that came out.
; Falls through print_digits and print_digit.
; ---------------------------------------------------------------------------

conv10          proc near
                xor     ax, ax
                mov     dl, al
                mov     cx, 16                  ; 16 bits

conv10_digit_lp:
                shl     bx, 1                   ; Next bit of BX into carry
                adc     al, al                  ; BCD result = result * 2 + bit
                daa
                xchg    al, ah
                adc     al, al
                daa
                xchg    al, ah
                adc     dl, dl                  ; Top digit, at most 6: no adjustment
                loop    conv10_digit_lp
                mov     bl, 10h                 ; Leading zero digit prints as 20h (blank)
                xchg    ax, dx                  ; AL = digit 4, DH = digits 3-2, DL = 1-0
                call    print_digit
                mov     al, dh
                call    print_digits
                mov     al, dl
conv10          endp

; Display the two BCD digits in AL (falls into print_digit for the low one)
print_digits    proc near
                mov     dh, al
                shr     al, 1
                shr     al, 1
                shr     al, 1
                shr     al, 1
                call    print_digit
                mov     al, dh
print_digits    endp

; Display the low nibble of AL as a digit.  BL = 10h while only leading zeros
; have been seen (printed as blanks); it is cleared by the first non zero digit.
print_digit     proc near
                and     al, 0Fh
                jz      short print_digit_ascii
                mov     bl, 0

print_digit_ascii:
                add     al, '0'
                sub     al, bl
                jmp     print_char
print_digit     endp

; ---------------------------------------------------------------------------
; L command: [n][,m]L
; List lines n to m.  Without n, start 11 lines before the current line (or
; at line 1); without m, list 23 lines.  A line that does not exist ends it.
; ---------------------------------------------------------------------------

list_cmd        proc near
                mov     bx, word ptr ds:[param1]
                or      bx, bx
                jnz     short list_setup
                mov     bx, word ptr ds:[curlin]
                sub     bx, list_before
                ja      short list_setup
                mov     bx, 1

list_setup:
                call    findlin
                jnz     short count_lines_ret   ; No such line
                mov     si, di
                mov     di, word ptr ds:[param2]
                inc     di
                sub     di, bx                  ; Number of lines = m + 1 - n
                ja      short print_line_check_rem
                mov     di, list_count          ; No usable m: default count
                jmp     short print_line_check_rem
list_cmd        endp

; ---------------------------------------------------------------------------
; Display lines, each preceded by its number (see shownum).
; Takes:   SI = address of the first line, BX = its number, DI = line count
;          (print_line enters with a count of 1)
; Returns: BX = number of the last line shown
; Control characters other than CR, LF and TAB are shown as ^ and a letter.
; ---------------------------------------------------------------------------

print_line      proc near
                mov     di, 1

print_line_check_rem:
                mov     cx, word ptr ds:[endtxt]
                sub     cx, si                  ; CX = bytes of text left
                jz      short print_line_ret
                mov     bp, word ptr ds:[curlin] ; TODO: dead code, BP is never used afterwards
                                                ; (shownum tests curlin itself).  Kept for byte parity.

print_line_hdr:
                push    cx
                call    shownum
                pop     cx

print_line_out_char:
                lodsb
                cmp     al, ' '
                jnb     short print_line_done   ; Printable
                cmp     al, lf
                jz      short print_line_done
                cmp     al, cr
                jz      short print_line_done
                cmp     al, tab
                jz      short print_line_done
                push    ax                      ; Other control character: show as
                mov     al, '^'                 ; ^ followed by the letter
                call    print_char
                pop     ax
                or      al, ctrlmask

print_line_done:
                call    print_char
                cmp     al, lf
                loopne  print_line_out_char     ; Until end of line or end of text
                jcxz    short print_line_ret
                inc     bx                      ; Next line
                dec     di
                jnz     short print_line_hdr
                dec     bx                      ; (BX = last line shown)

print_line_ret:
                ret
print_line      endp

; ---------------------------------------------------------------------------
; Copy a line into the line editor buffer.
; Takes:   SI = address of the line
; Returns: DX = line length without the CR, SI = address of the LF after it,
;          editbuf_text = the line ending with a CR, editbuf_len = length
; A line longer than the buffer is cut to 254 characters (DX keeps the true
; length) and still ends with a CR in the buffer.
; ---------------------------------------------------------------------------

linelen         proc near
                mov     di, offset editbuf_text
                mov     cx, editbuf_max
                mov     dx, -1                  ; Becomes 0 on the first character

linelen_lp:
                lodsb
                stosb
                inc     dx
                cmp     al, cr
                loopne  linelen_lp
                mov     byte ptr ds:[editbuf_len], dl
                jz      short print_line_ret    ; CR found: line fits in the buffer

linelen_done:
                lodsb                           ; Skip the rest of a long line
                inc     dx
                cmp     al, cr
                jnz     short linelen_done
                dec     di                      ; Overwrite the last copied character
                stosb                           ; with the CR
                ret
linelen         endp

; ---------------------------------------------------------------------------

replace_notfnd:
                jmp     notfound

; ---------------------------------------------------------------------------
; R command: [n][,m][?]Rsearch^Zreplace
; Replace every occurrence of "search" by "replace" in lines n to m (default:
; the whole text).  Each changed line is displayed, with ? the user is asked
; "O.K.?" first.  The new line may not exceed 254 characters.
; Match state (srch_ptr, srch_lineptr, srch_lineno, srch_remaining) is kept
; by find_match.
; ---------------------------------------------------------------------------

replace_cmd     proc near
                call    parse_search_args
                jnz     short replace_notfnd

replace_lp:
                mov     si, word ptr ds:[srch_lineptr]
                call    linelen                 ; DX = old line length
                sub     dx, word ptr ds:[srch_len]
                mov     cx, word ptr ds:[rplc_len]
                add     dx, cx                  ; DX = new line length
                cmp     dx, max_linelen
                ja      short replace_toolng
                mov     bx, word ptr ds:[srch_lineno]
                push    dx
                call    shownum
                pop     dx
                mov     cx, word ptr ds:[srch_ptr] ; Show the new line: the text
                mov     si, word ptr ds:[srch_lineptr] ; before the match,
                sub     cx, si
                dec     cx
                call    print_string
                push    si
                mov     si, offset rplc_buf     ; the replacement,
                mov     cx, word ptr ds:[rplc_len]
                call    print_string
                pop     si
                add     si, word ptr ds:[srch_len] ; and the text after the match
                mov     cx, dx                  ; (DX counts what is left, + CR LF)
                add     cx, 2
                call    print_string
                call    prompt_yesno
                jnz     short replace_next      ; Answered no
                call    set_curlin
                mov     di, word ptr ds:[srch_ptr]
                dec     di                      ; DI = start of the match
                mov     si, offset rplc_buf
                mov     dx, word ptr ds:[srch_len] ; Old length
                mov     cx, word ptr ds:[rplc_len] ; New length
                dec     cx
                add     word ptr ds:[srch_ptr], cx ; Continue the search after the
                inc     cx                      ; replacement
                dec     dx
                sub     word ptr ds:[srch_remaining], dx
                jnb     short replace_shift
                mov     word ptr ds:[srch_remaining], 0

replace_shift:
                inc     dx
                call    shift_text              ; Replace DX bytes at DI by CX from SI

replace_next:
                call    find_match
                jnz     short print_string_ret        ; No more matches
                jmpn    replace_lp
replace_cmd     endp

; ---------------------------------------------------------------------------
; Display CX characters at SI, counting them down in DX as well.
; ---------------------------------------------------------------------------

print_string    proc near
                jcxz    short print_string_ret

print_str_lp:
                lodsb
                call    print_char
                dec     dx
                loop    print_str_lp

print_string_ret:
                ret
print_string    endp

; ---------------------------------------------------------------------------

replace_toolng:
                mov     dx, offset toolng       ; "Line too long"
                jmp     short print_msg

; ---------------------------------------------------------------------------
; S command: [n][,m][?]Ssearch
; Search lines n to m (default: the whole text) for "search".  Each line
; found is displayed.  Without ? the first one becomes the current line and
; the search ends; with ? the user is asked "O.K.?" and a "no" goes on to the
; next line.  "Not found" is displayed when there is nothing (more) to find.
; ---------------------------------------------------------------------------

search_cmd      proc near
                call    parse_search_args
                jnz     short notfound

search_lp:
                mov     bx, word ptr ds:[srch_lineno]
                mov     si, word ptr ds:[srch_lineptr]
                call    print_line              ; Show the line holding the match
                call    prompt_yesno
                jz      short set_curlin        ; Accepted: it becomes the current line
                mov     di, word ptr ds:[srch_ptr] ; Refused: go to the next line
                mov     cx, word ptr ds:[srch_remaining]
                mov     al, lf
                repne scasb
                jnz     short notfound
                mov     word ptr ds:[srch_ptr], di
                mov     word ptr ds:[srch_lineptr], di
                mov     word ptr ds:[srch_remaining], cx
                inc     word ptr ds:[srch_lineno]
                call    find_match
                jz      short search_lp

notfound:
                mov     dx, offset notfnd       ; "Not found"

; Display the '$' terminated message at DX and return to the caller
print_msg:
                ; CPM86 PORT: AH= -> CL=
                mov     cl, bdos_print                   ; DOS: print string
                int     0E0h            ; CPM86 PORT: int 21h -> int 0E0h
                ret
search_cmd      endp

; ---------------------------------------------------------------------------
; Make the line of the current match the current line
; ---------------------------------------------------------------------------

set_curlin      proc near
                mov     ax, word ptr ds:[srch_lineptr]
                mov     word ptr ds:[pointer], ax
                mov     ax, word ptr ds:[srch_lineno]
                mov     word ptr ds:[curlin], ax

set_curlin_ret:
                ret
set_curlin      endp

; ---------------------------------------------------------------------------
; Ask "O.K.? " when the ? option was given.
; Returns: ZF set = go ahead (also always when there was no ?), ZF clear = no.
; CR, "Y" and "y" mean yes, any other key means no.
; ---------------------------------------------------------------------------

prompt_yesno    proc near
                test    byte ptr ds:[qflg], 0FFh
                jz      short set_curlin_ret    ; No ?, ZF is set
                mov     dx, offset prompt_ok
                ; CPM86 PORT: AH= -> CL=
                mov     cl, bdos_print                   ; DOS: print string
                int     0E0h            ; CPM86 PORT: int 21h -> int 0E0h
                ; CPM86 PORT: AH= -> CL=
                mov     cl, bdos_kbd_echo       ; BDOS: keyboard input with echo
                int     0E0h
                push    ax
                call    print_crlf
                pop     ax
                cmp     al, cr
                jz      short set_curlin_ret
                cmp     al, 'Y'
                jz      short set_curlin_ret
                cmp     al, 'y'                 ; ZF set if "y"

prompt_yesno_ret:
                ret
prompt_yesno    endp

; ---------------------------------------------------------------------------
; Parse the arguments of the S and R commands and find the first match.
; Takes:   SI = command line text after the command letter: the search
;          string, then optionally ^Z and the replacement string, up to CR
; Returns: ZF set if a match was found (see find_match for the state), ZF
;          clear if the search string is empty or the range holds no match
; Sets srch_len, rplc_len, and the search range: from line param1 (default 1)
; to line param2 (default the end of the text).
; ---------------------------------------------------------------------------

parse_search_args proc near
                mov     di, offset srch_buf
                call    get_param_str
                or      al, al                  ; ZF = 0 for the error returns below
                jcxz    short prompt_yesno_ret    ; Empty search string
                mov     word ptr ds:[srch_len], cx
                xor     cx, cx
                cmp     al, cr
                jz      short parse_srch_find_range ; No replacement string
                mov     di, offset rplc_buf
                call    get_param_str

parse_srch_find_range:
                mov     word ptr ds:[rplc_len], cx
                mov     bx, word ptr ds:[param1]
                cmp     bx, 1
                adc     bx, 0                   ; 0 becomes 1
                call    findlin
                mov     word ptr ds:[srch_ptr], di ; Start of the range
                mov     word ptr ds:[srch_lineptr], di
                mov     word ptr ds:[srch_lineno], dx
                mov     bx, word ptr ds:[param2]
                cmp     bx, 1
                sbb     bx, -1                  ; param2 + 1, but 0 stays 0 (the end)
                call    findlin                 ; DI = end of the range
                mov     cx, di
                sub     cx, word ptr ds:[srch_ptr] ; CX = size of the range
                or      al, 0FFh                ; ZF = 0 for the error returns
                jcxz    short prompt_yesno_ret    ; Empty range
                sub     cx, word ptr ds:[srch_len]
                jb      short prompt_yesno_ret    ; Range shorter than the string
                inc     cx                      ; Possible start positions
                mov     word ptr ds:[srch_remaining], cx
parse_search_args endp

; ---------------------------------------------------------------------------
; Find the next occurrence of the search string.
; Takes:   srch_ptr = where to resume (just after the first character of the
;          previous match, or the start of the range), srch_remaining = number
;          of start positions left to try, srch_lineptr / srch_lineno = the
;          line that contains srch_ptr
; Returns: ZF set if found, with srch_ptr = address after the first character
;          of the match, srch_remaining = positions left after it, and
;          srch_lineptr / srch_lineno = start and number of the match's line;
;          ZF clear if there is no further match
; ---------------------------------------------------------------------------

find_match      proc near
                mov     al, ds:[srch_buf]       ; First character of the string
                mov     cx, word ptr ds:[srch_remaining]
                mov     di, word ptr ds:[srch_ptr]

find_match_lp:
                or      di, di                  ; ZF = 0, in case CX = 0
                repne scasb                     ; Look for the first character
                jnz     short prompt_yesno_ret
                mov     dx, cx                  ; Save the position
                mov     bx, di
                mov     cx, word ptr ds:[srch_len]
                dec     cx
                mov     si, offset srch_buf + 1
                cmp     al, al                  ; ZF = 1, in case CX = 0
                repe cmpsb                      ; Compare the rest of the string
                mov     cx, dx
                mov     di, bx
                jnz     short find_match_lp     ; Not equal: keep looking
                mov     word ptr ds:[srch_remaining], cx
                mov     cx, di
                mov     word ptr ds:[srch_ptr], di
                mov     di, word ptr ds:[srch_lineptr]
                sub     cx, di                  ; Bytes from the line start to the match
                mov     al, lf
                mov     dx, word ptr ds:[srch_lineno]

find_match_count_lines:
                inc     dx                      ; Count the line starts up to the match,
                mov     bx, di                  ; BX = start of the line being scanned
                repne scasb
                jz      short find_match_count_lines
                dec     dx                      ; The last scan found no LF
                mov     word ptr ds:[srch_lineno], dx
                mov     word ptr ds:[srch_lineptr], bx
                xor     al, al                  ; ZF = 1, found

find_match_ret:
                ret
find_match      endp

; ---------------------------------------------------------------------------
; Copy a string argument from the command line.
; Takes:   SI = command line, DI = destination
; Returns: CX = length, AL = the terminator (CR or ^Z), which is not copied
; ---------------------------------------------------------------------------

get_param_str   proc near
                xor     cx, cx

get_param_char_lp:
                lodsb
                cmp     al, ctrlz
                jz      short find_match_ret
                cmp     al, cr
                jz      short find_match_ret
                stosb
                inc     cx
                jmp     short get_param_char_lp
get_param_str   endp

; ---------------------------------------------------------------------------
; D command: [n][,m]D
; Delete lines n to m.  Without n, start at the current line; without m,
; delete only line n.  The line after the deleted ones becomes the current
; line (same number as the first deleted line).
; ---------------------------------------------------------------------------

delete_cmd      proc near
                mov     bx, word ptr ds:[param1]
                or      bx, bx
                jnz     short delete_find_start
                mov     bx, word ptr ds:[curlin]

delete_find_start:
                call    findlin
                jnz     short find_match_ret     ; No such line
                push    bx                      ; First line number
                push    di                      ; and its address
                mov     bx, word ptr ds:[param2]
                or      bx, bx
                jnz     short delete_find_end
                mov     bx, dx                  ; No m: just this line

delete_find_end:
                inc     bx                      ; Line after the last one to delete
                call    findlin
                mov     dx, di
                pop     di
                sub     dx, di                  ; DX = bytes to delete
                jbe     short delete_comerr     ; m before n
                pop     word ptr ds:[curlin]
                mov     word ptr ds:[pointer], di
                xor     cx, cx                  ; Replace DX bytes by nothing
                jmp     short shift_text

delete_comerr:
                jmp     comerr
delete_cmd      endp

; ---------------------------------------------------------------------------
; Line edit: [n]  (a command line with no command letter)
; Show line n (default: the line after the current one) and read a new
; version of it.  The old text is available for the DOS editing keys (the
; line is copied into the input buffer).  An empty answer leaves the line as
; it is.  Falls into shift_text to replace the old text by the new one.
; ---------------------------------------------------------------------------

edit_cmd        proc near
                mov     bx, word ptr ds:[param1]
                or      bx, bx
                jnz     short edit_find_line
                mov     bx, word ptr ds:[curlin]
                inc     bx

edit_find_line:
                call    findlin
                mov     si, di
                mov     word ptr ds:[curlin], dx
                mov     word ptr ds:[pointer], si
                jnz     short find_match_ret     ; Past the end of the text
                cmp     si, word ptr ds:[endtxt]
                jz      short find_match_ret     ; The empty line at the end
                call    linelen                 ; Old line into the input buffer
                mov     word ptr ds:[srch_len], dx ; Old length (srch_len is free here)
                mov     si, word ptr ds:[pointer]
                call    print_line
                call    shownum
                ; CPM86 PORT: AH= -> CL=; push SI (BDOS clobbers it, SI=line pointer)
                push    si
                mov     cl, bdos_bufin          ; BDOS: buffered keyboard input
                mov     dx, offset editbuf
                int     0E0h
                pop     si
                mov     al, lf
                call    print_char
                mov     cl, byte ptr ds:[editbuf_len]
                mov     ch, 0
                jcxz    short shift_ret         ; Nothing typed: keep the old line
                mov     dx, word ptr ds:[srch_len] ; Old length
                mov     si, offset editbuf_text ; New text
                mov     di, word ptr ds:[pointer]
edit_cmd        endp

; ---------------------------------------------------------------------------
; Replace text in the buffer: remove DX bytes at DI and put CX bytes from SI
; in their place, moving the rest of the text (and its ^Z) to suit.
; Takes:   DI = address, DX = old length, CX = new length, SI = new text
; If the text would no longer fit in memory, memerr is taken.  Used by the
; D command (jumps here), R (calls) and the line edit command (falls in).
; ---------------------------------------------------------------------------

shift_text      proc near
                cmp     cx, dx
                jz      short shift_copy        ; Same length: just copy
                push    si
                push    di
                push    cx
                mov     si, di
                add     si, dx                  ; SI = start of the text after the old bytes
                add     di, cx                  ; DI = where it goes after the new bytes
                mov     ax, word ptr ds:[endtxt]
                sub     ax, dx
                add     ax, cx                  ; AX = new end of the text
                cmp     ax, word ptr ds:[mem_top]
                jnb     short memerr
                xchg    ax, word ptr ds:[endtxt] ; Store it, AX = old end
                mov     cx, ax
                sub     cx, si                  ; CX = length of the moved text - 1
                cmp     si, di
                ja      short shift_backward    ; Moving down: copy forwards
                add     si, cx                  ; Moving up: copy backwards from the end
                add     di, cx
                std

shift_backward:
                inc     cx
                rep movsb                       ; Includes the final ^Z
                cld
                pop     cx
                pop     di
                pop     si

shift_copy:
                rep movsb                       ; Copy the new bytes

shift_ret:
                ret

; "Insufficient memory": give up the command (the stack is reset at the prompt)
memerr:
                mov     dx, offset memful
                ; CPM86 PORT: AH= -> CL=
                mov     cl, bdos_print                   ; DOS: print string
                int     0E0h            ; CPM86 PORT: int 21h -> int 0E0h
                jmp     command
shift_text      endp

; ---------------------------------------------------------------------------
; I command: [n]I
; Insert lines before line n (default: the current line).  Lines are typed
; one at a time, each prompted with its number, until a line starting with ^Z
; or ^Break is typed.  The current line afterwards is the one after the new
; lines.
; Method: the text from line n to the end is first moved to the top of
; memory, so the new lines are written straight into the gap after the lines
; before n.  When done, the moved text is copied back down after them.
; ---------------------------------------------------------------------------

insert_cmd      proc near
                ; CPM86 PORT: ^C vector install removed -- no equivalent; ^C triggers warm boot
                mov     bx, word ptr ds:[param1]
                or      bx, bx
                jnz     short insert_find_pos
                mov     bx, word ptr ds:[curlin]

insert_find_pos:
                call    findlin                 ; DI = where the new lines go
                mov     cx, word ptr ds:[endtxt]
                mov     si, cx
                sub     cx, di
                inc     cx                      ; Text after DI, and its ^Z
                mov     di, word ptr ds:[mem_top]
                std
                rep movsb                       ; Move it up to the top of memory
                xchg    di, si
                cld
                inc     di                      ; DI = insertion point
                mov     bp, si                  ; BP = last byte of the gap
                mov     bx, dx                  ; BX = number of the new line

insert_lp:
                mov     word ptr ds:[pointer], di
                mov     word ptr ds:[curlin], bx
                mov     word ptr ds:[endtxt], bp ; Text seems to end at the gap
                call    shownum
                ; CPM86 PORT: AH= -> CL=; push BX/DI/BP (BDOS clobbers all; BX=line#, DI=insert pt, BP=gap end)
                push    bx
                push    di
                push    bp
                mov     cl, bdos_bufin          ; BDOS: buffered keyboard input
                mov     dx, offset editbuf
                int     0E0h
                pop     bp
                pop     di
                pop     bx
                call    print_lf
                mov     si, offset editbuf_text
                cmp82_si ctrlz                  ; ^Z at the start: finished
                jz      short insert_done       ; ^Z at the start: finished
                mov     cl, [si-1]              ; Length typed
                mov     ch, 0
                mov     dx, si
                add     dx, cx
                inc     dx
                cmp     dx, bp                  ; TODO: room check, see the note below
                jnb     short memerr
                rep movsb                       ; Copy the line,
                movsb                           ; its CR,
                mov     al, lf
                stosb                           ; and add a LF
                inc     bx
                jmp     short insert_lp

; TODO: the room check above is wrong in the original program (confirmed
; with emu2, see below).  DX = SI + length + 1 where SI is the address in the
; input buffer (editbuf_text, around 0C20h), not the destination DI, and BP
; (the end of the gap) is always at or above buf_start.  The test can
; therefore hardly ever fire, so an insert into a nearly full buffer runs past
; BP and overwrites the text that was moved to the top of memory.
; Fix, same size (2 bytes), different binary:
;                 mov     dx, di          ; instead of: mov     dx, si
; (then DX = DI + length + 1 is compared with BP and memerr is taken when the
; new line, with its CR LF, would not fit in the gap).
; Test (emu2, copy of edlin.com with the buffer cut to 2000 bytes and "." instead
; of ^Z ending the insert; the check itself untouched): 1I on a 640 byte file
; and 60 lines of 32 bytes, 42 of which fit.  Original: no message, all 60 lines
; accepted and the 20 original lines destroyed.  With the fix: "Insufficient
; memory" after line 43.

; ^Break while inserting: rebuild the segments and stack, then finish as for ^Z
break_ins:
                mov     ax, cs
                mov     ds, ax
                mov     es, ax
                mov     ss, ax
                mov     sp, offset stack_top
                call    print_crlf

insert_done:
                mov     bp, word ptr ds:[endtxt]
                mov     di, word ptr ds:[pointer] ; End of the new lines
                mov     si, bp
                inc     si                      ; Moved text at the top of memory
                mov     cx, word ptr ds:[mem_top]
                sub     cx, bp
                rep movsb                       ; Bring it back down
                dec     di
                mov     word ptr ds:[endtxt], di ; DI is at the ^Z
                jmp     command
insert_cmd      endp

; ---------------------------------------------------------------------------
; Q command: Q
; Quit without saving, after "Abort edit (Y/N)?" is answered with Y or y.
; Any other answer returns to the prompt.  The temporary file is deleted and
; the original file is left as it was.
; ---------------------------------------------------------------------------

quit_cmd        proc near
                mov     dx, offset abort_prompt
                ; CPM86 PORT: AH= -> CL=
                mov     cl, bdos_print                   ; DOS: print string
                int     0E0h            ; CPM86 PORT: int 21h -> int 0E0h
                ; CPM86 PORT: AH= -> CL=
                mov     cl, bdos_kbd_echo       ; BDOS: keyboard input with echo
                int     0E0h
                and     al, upmask              ; Upper case
                cmp     al, 'Y'
                jz      short quit_is_y         ; ST4: print_crlf now out of short range
                jmp     print_crlf              ; No: new line, back to the prompt
quit_is_y:
                mov     dx, offset fcb2
                ; CPM86 PORT: AH= -> CL=
                mov     cl, bdos_close          ; BDOS: close the temporary file
                int     0E0h
                ; CPM86 PORT: AH= -> CL=
                mov     cl, bdos_delete         ; BDOS: delete it (DX is unchanged)
                int     0E0h
                ; CPM86 PORT: int 20h -> BDOS fn 0 (warm boot / return to CCP)
                xor     cx, cx
                mov     dl, 0
                int     0E0h
quit_cmd        endp

; ---------------------------------------------------------------------------
; E command: E
; End the edit: write all the text (reading and writing the rest of the file
; first if it did not all fit in memory), terminate the new file with a ^Z,
; rename the original file to filename.BAK and the temporary file filename.$$$
; to the original name, then return to DOS.
; modflg is set so that append_cmd does not complain about lack of memory
; while the file is being copied over (checked with emu2: a plain nnnA that
; cannot read n lines says "Insufficient memory", E on the same file does not).
; "End of input file" is displayed when the copy reaches the end of the file.
; ---------------------------------------------------------------------------

exit_all_recs:
                mov     word ptr ds:[param1], all_lines
                call    append_cmd              ; Read as much of the file as fits
                                                ; and go round again
exit_cmd        proc near
                mov     byte ptr ds:[modflg], 1
                mov     bx, all_lines
                call    write_recs              ; Write all the text in memory
                test    byte ptr ds:[newfile_flg], 0FFh
                jz      short exit_all_recs     ; The file is not all copied yet
                ; CPM86 PORT ST4: write final ^Z record (1 x 128 bytes, CP/M-86 pads remainder with ^Z)
                mov     di, word ptr ds:[endtxt] ; DMA start = address of the ^Z
                mov     ax, 1                   ; 1 record
                mov     dx, offset fcb2
                call    write_block
                ; CPM86 PORT: AH= -> CL=
                mov     cl, bdos_close          ; BDOS: close the new file
                int     0E0h
                mov     si, fcb1                ; Rename the original file to name.BAK:
                lea     di, [si+fcb_newname]    ; the new name field of FCB 1 gets
                mov     dx, si                  ; the drive and name,
                mov     cx, fcb_drvname
                rep movsb
                mov     si, offset bak          ; and the extension BAK
                movsw
                movsb
                ; CPM86 PORT: AH= -> CL=
                mov     cl, bdos_rename         ; BDOS: rename file (FCB at DX)
                int     0E0h
                mov     si, fcb1                ; Rename name.$$$ to the original name:
                mov     di, offset fcb2_newname ; copy drive, name and extension of
                mov     cx, fcb_fname_words     ; FCB 1 (12 bytes) to the new name
                rep movsw                       ; field of FCB 2
                mov     dx, offset fcb2
                ; CPM86 PORT: AH= -> CL= (AH still holds bdos_rename from above)
                mov     cl, bdos_rename         ; BDOS: rename $$$ to original name
                int     0E0h
                ; CPM86 PORT: int 20h -> BDOS fn 0 (warm boot / return to CCP)
                xor     cx, cx
                mov     dl, 0
                int     0E0h
exit_cmd        endp

; ---------------------------------------------------------------------------
; CPM86 PORT ST4: read_block / write_block
; CP/M-86 BDOS has no multi-record block I/O (DOS fn 27h/28h have no equivalent).
; These procs loop calling fn 21h (F_READRAND) / fn 22h (F_WRITERAND) one record
; at a time, advancing the DMA address and fcb_rr manually each iteration.
;
; read_block
;   In:  AX = record count, DX = FCB address (fcb1), DI = DMA start offset
;   Out: CX = bytes placed in buffer (records_read * 128), AL = last BDOS status
;        (0 = all records read; non-zero = EOF/error, read stopped early)
;        DI preserved (restored); BX, SI, ES preserved
;   fcb1_rr is incremented once per record successfully read.
; ---------------------------------------------------------------------------

read_block      proc near
                push    bx
                push    di
                mov     bx, ax              ; BX = records to read
                xor     ax, ax              ; AX = records read so far
                                            ; (no BH init -- see rdblk_eof for AL save)
rdblk_lp:
                or      bx, bx
                jz      short rdblk_done
                push    ax
                push    bx
                push    di                  ; save running buffer pointer (BDOS clobbers DI)
                push    dx                  ; FCB address
                mov     dx, di              ; DMA = current buffer position
                mov     cl, bdos_set_dta
                int     0E0h
                pop     dx                  ; restore FCB address
                mov     cl, bdos_rdrand     ; BDOS fn 21h: F_READRAND
                int     0E0h
                push    ax                  ; save fn21h result (AL) on stack
                pop     cx                  ; CX = fn21h result (CH=0, CL=AL)
                pop     di                  ; restore buffer pointer
                pop     bx
                pop     ax                  ; restore records_read
                or      cl, cl              ; test fn21h result
                jnz     short rdblk_eof     ; EOF or error: stop
                inc     word ptr ds:[fcb1_rr]
                adc     word ptr ds:[fcb1_rr+2], 0
                add     di, 128             ; advance to next record slot
                inc     ax
                dec     bx
                jmp     rdblk_lp
rdblk_eof:
                ; CL = fn21h status; AX = records_read (clean, not clobbered)
                push    cx                  ; save CL = fn21h status
                mov     cl, 7
                shl     ax, cl              ; AX = records_read * 128 = bytes placed
                mov     cx, ax              ; return bytes in CX
                pop     ax                  ; restore: AL = fn21h status (was CL)
                pop     di                  ; restore DI to entry value
                pop     bx
                ret
rdblk_done:
                xor     al, al              ; AL = 0: all records read successfully
                mov     cl, 7
                shl     ax, cl              ; AX = records_read * 128 = bytes placed
                mov     cx, ax
                pop     di
                pop     bx
                ret
read_block      endp

; ---------------------------------------------------------------------------
; write_block
;   In:  AX = record count, DX = FCB address (fcb2), DI = DMA start offset
;   Out: AL = last BDOS status (0 = ok, non-zero = disk full/error)
;        DI preserved; BX preserved
;   fcb2_rr is incremented once per record successfully written.
; ---------------------------------------------------------------------------

write_block     proc near
                push    bx
                push    di
                mov     bx, ax              ; BX = records to write
                xor     al, al              ; AL = last BDOS status
wblk_lp:
                or      bx, bx
                jz      short wblk_done
                push    bx
                push    di                  ; save running buffer pointer (BDOS clobbers DI)
                push    dx                  ; FCB address
                mov     dx, di              ; DMA = current buffer position
                mov     cl, bdos_set_dta
                int     0E0h
                pop     dx                  ; restore FCB address
                mov     cl, bdos_wrrand     ; BDOS fn 22h: F_WRITERAND
                int     0E0h
                pop     di                  ; restore buffer pointer
                pop     bx
                or      al, al
                jnz     short wblk_done     ; disk full or error: stop
                inc     word ptr ds:[fcb2_rr]
                adc     word ptr ds:[fcb2_rr+2], 0
                add     di, 128             ; advance to next record slot
                dec     bx
                jmp     wblk_lp
wblk_done:
                pop     di                  ; restore DI
                pop     bx
                ret
write_block     endp


; ---------------------------------------------------------------------------
; Console output.  The three entry points fall through into each other.
; print_crlf: write CR, then LF       print_lf: write LF
; print_char: write the character in AL (AX and DX are preserved)
; ---------------------------------------------------------------------------

print_crlf      proc near
                mov     al, cr
                call    print_char
print_crlf      endp

print_lf        proc near
                mov     al, lf
print_lf        endp

print_char      proc near
                ; CPM86 PORT: int 21h AH= -> int 0E0h CL=; also save BX/SI/DI/ES (BDOS clobbers all)
                push    bx
                push    si
                push    di
                push    es
                push    dx
                xchg    ax, dx                  ; DL = character
                mov     cl, bdos_display        ; BDOS: console output
                int     0E0h
                xchg    ax, dx                  ; Restore AX
                pop     dx
                pop     es
                pop     di
                pop     si
                pop     bx
                ret
print_char      endp

; ---------------------------------------------------------------------------
; ^C handler (interrupt 23h, set at the command prompt).  DOS enters with
; unknown registers: rebuild the segments and stack, then return to the prompt.
; ---------------------------------------------------------------------------
; CPM86 PORT: break_cmd / break_ins are unreachable under CP/M-86

break_cmd:
                mov     ax, cs
                mov     ds, ax
                mov     es, ax
                mov     ss, ax
                mov     sp, offset stack_top
                call    print_crlf
                jmp     command

; ---------------------------------------------------------------------------
; Message strings, '$' terminated for DOS function 09h.  They start at 093Eh
; and "bak" doubles as the BAK extension string.
; ---------------------------------------------------------------------------

bak             db      "BAK"
baddrv          db      "Invalid drive or file name", "$"
nofile          db      "File name must be specified", "$"
nobak           db      "Cannot edit .BAK file--rename file", "$"
nodir           db      "No room in directory for file", "$"
dskful          db      "Disk full--file write not completed", "$"
memful          db      cr, lf, "Insufficient memory", cr, lf, "$"
badcom          db      "Entry error", cr, lf, "$"
newfil          db      "New file", cr, lf, "$"
notfnd          db      "Not found", cr, lf, "$"
prompt_ok       db      "O.K.? $"
toolng          db      "Line too long", cr, lf, "$"
eofmsg          db      "End of input file", cr, lf, "$"
abort_prompt    db      "Abort edit (Y/N)? $"

; ===========================================================================
; Uninitialised work area, past the end of the COM image (0A58h and up).
; Nothing here is stored in the file; it is addressed with equates only.
; ===========================================================================


; FCB 2: the temporary file filename.$$$ (standard 37 byte FCB layout)
fcb2            label   byte                    ; 0A58h
fcb2_ext        equ     fcb2 + 9                ; Extension
fcb2_recsiz     equ     fcb2 + 14               ; Record size word (0A66h)
fcb2_newname    equ     fcb2 + fcb_newname      ; Rename: new name field (0A68h)
fcb2_rr         equ     fcb2 + 33               ; Random record number dword (0A79h)

; Flags
qflg            equ     fcb2 + 37               ; Byte: "?" typed, confirm each change (0A7Dh)
newfile_flg     equ     fcb2 + 38               ; Byte: non zero = no more to read from the file,
                                                ; 0FFh right after the open of a new file (0A7Eh)
modflg          equ     fcb2 + 39               ; Byte: 1 once the E command has started (0A7Fh)

; Command arguments and search state
param1          equ     fcb2 + 40               ; Word: first number on the command line (0A80h)
param2          equ     param1 + 2              ; Word: second number (0A82h)
srch_len        equ     param2 + 2              ; Word: search string length (0A84h); the line
                                                ; edit also keeps the old line length here
rplc_len        equ     srch_len + 2            ; Word: replacement string length (0A86h)
srch_ptr        equ     rplc_len + 2            ; Word: address after the 1st char of the match (0A88h)
srch_lineno     equ     srch_ptr + 2            ; Word: number of the line holding the match (0A8Ah)
srch_lineptr    equ     srch_lineno + 2         ; Word: address of that line (0A8Ch)
srch_remaining  equ     srch_lineptr + 2        ; Word: bytes left to search (0A8Eh)

; Text buffer state
curlin          equ     srch_remaining + 2      ; Word: current line number (0A90h)
pointer         equ     curlin + 2              ; Word: address of the current line (0A92h)
buf_1qtr        equ     pointer + 2             ; Word: 1/4 of the buffer size in bytes (0A94h)
buf_3qtr        equ     buf_1qtr + 2            ; Word: address 3/4 into the buffer (0A96h)
mem_top         equ     buf_3qtr + 2            ; Word: last usable address (0A98h)
endtxt          equ     mem_top + 2             ; Word: address of the ^Z ending the text (0A9Ah)

; Buffers (each DOS buffered input buffer starts with a max length byte and
; a count byte)
combuf          equ     endtxt + 2              ; Command line, 130 bytes (0A9Ch)
srch_buf        equ     combuf + combuf_size + 2 ; Search string, 128 bytes (0B1Eh)
rplc_buf        equ     srch_buf + strbuf_size  ; Replacement string, 128 bytes (0B9Eh)
editbuf         equ     rplc_buf + strbuf_size  ; Line being edited, 258 bytes (0C1Eh)
editbuf_len     equ     editbuf + 1             ; Its length byte (0C1Fh)
editbuf_text    equ     editbuf + 2             ; Its text (0C20h)
stack_bot       equ     editbuf + editbuf_max + 3 ; End of the buffer (0D20h)
stack_top       equ     stack_bot + stack_size  ; Initial SP, stack grows down (0D48h)
buf_start       equ     stack_top + 1           ; Text buffer; stack_top holds a LF (0D49h)

program         ends
                end     _start
