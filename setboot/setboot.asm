; --------------------------------------------------------------------
; SETBOOT -- primary MBR partition selector, IBM PC, 8086/8088.
; MASM 1.10. Same payload in DOS COM and CP/M-86 8080-model CMD.
; Written independently; ccpmio.a86 and setboot.dis are reference
; only.
;
; Syntax: SETBOOT [disk:] [partition] [/R | [R | [R]]
; Physical disk 1 = BIOS 80h. Primary slots retain their MBR
; numbers 1-4.
; No extended-chain traversal, partition boot, or partition-table
; rebuilding.
;
; CCP/M 3.1 IBM PC: SYSDAT +28h gives XIOS entry; XIOS 14/CX=1
; gives the setup block with the original INT 13h vector at +0/+2.
; Use that vector under MXdisk/P_KEEP/NoSwitch, because logical XIOS
; reads are relative to the CP/M partition and cannot read the MBR.
; --------------------------------------------------------------------

; Packed partition-type row: byte ID followed by word string offset.
ptype                   macro   type_code,type_text
                        db      type_code
                        dw      type_text
                        endm

program                 segment
                        assume  cs:program,ds:program,es:program,ss:program
                        org     100h
entry:
                        jmp     near ptr start

OS_DOS                  equ     0
OS_CPM                  equ     1
OS_CCPM                 equ     2

; Runtime state.
os_kind                 db      OS_DOS
cpm_version             dw      0
chosen                  db      0
reboot                  db      0
disk_unit               db      0
disk_seen               db      0
show_usage              db      0
listed                  db      0
type_id                 db      0
tries                   db      0
io_function             db      0
io_error                db      0
dma_seg                 dw      0

; CCP/M pointers and acquired-resource state.
sys_seg                 dw      0
pd_off                  dw      0
uda_seg                 dw      0
vc_off                  dw      0
xios_vector             dd      0
bios_vector             dd      0
saved_keep              dw      0
saved_priority          db      0
process_reserved        db      0
console_reserved        db      0
disk_reserved           db      0

; CCP/M user queue control block: link, name, message length, buffer.
queue_block             db      8 dup (0),'MXdisk  ',6 dup (0)

; Console text.
newline                 db      13,10,'$'
msg_space               db      '  $'
msg_disk                db      'Physical disk $'
msg_columns             db      13,10,'Slot Active Type Start-LBA Sectors Name',13,10,'$'
msg_legend              db      '* = active. Counts are 512-byte sectors. Logical '
                        db      'partitions are not listed.',13,10,'$'
msg_no_parts            db      'No nonempty primary partitions.',13,10,'$'
msg_already             db      'Selected primary partition is already active.',13,10,'$'
msg_activated           db      'Primary partition activated; MBR write verified.',13,10,'$'
msg_reboot              db      'Rebooting...',13,10,'$'

; Errors.
err_args                db      'Invalid parameters.',13,10,'$'
err_read                db      'Cannot read MBR: disk absent or BIOS read error.',13,10,'$'
err_disk                db      'Physical disk does not exist.',13,10,'$'
err_signature           db      'Missing MBR signature (55 AA).',13,10,'$'
err_empty               db      'Selected primary slot is empty.',13,10,'$'
err_container           db      'Selected slot is a container; it cannot be activated.',13,10,'$'
err_table               db      'Invalid primary table: flags, extent, or overlap. '
                        db      'No write made.',13,10,'$'
err_write               db      'MBR write failed; disk state is uncertain. No reboot.',13,10,'$'
err_verify_read         db      'Cannot read back written MBR; disk state is '
                        db      'uncertain. No reboot.',13,10,'$'
err_verify              db      'Written MBR does not match; disk state is '
                        db      'uncertain. No reboot.',13,10,'$'
err_busy                db      'CCP/M requires an unbuffered local console and no '
                        db      'open files.',13,10,'$'
err_queue               db      'Cannot acquire CCP/M MXdisk queue.',13,10,'$'
err_release             db      'Cannot release CCP/M MXdisk queue. No reboot.',13,10,'$'

; OS-specific command syntax.
usage_dos               db      'Usage: SETBOOT [disk:] [partition] [/R]',13,10
                        db      '  disk: physical disk 1..128 (default 1); '
                        db      'partition: primary slot 1..4.',13,10
                        db      '  No partition: list only. /R: reboot after '
                        db      'successful activation.',13,10
                        db      '  Examples: SETBOOT   SETBOOT 2:   SETBOOT 2: 3 /R',13,10,'$'

usage_cpm               db      'Usage: SETBOOT [disk:] [partition] [R]',13,10
                        db      '  disk: physical disk 1..128 (default 1); '
                        db      'partition: primary slot 1..4.',13,10
                        db      '  No partition: list only. [R] (or [R): reboot '
                        db      'after successful activation.',13,10
                        db      '  Examples: SETBOOT   SETBOOT 2:   SETBOOT 2: 3 [R]',13,10,'$'

; Names are identification hints, not proof of boot code or filesystem health.
types                   label   byte
                        ptype   01h,    name_fat12
                        ptype   04h,    name_fat16s
                        ptype   05h,    name_extended
                        ptype   06h,    name_fat16
                        ptype   07h,    name_hpfs
                        ptype   0bh,    name_fat32
                        ptype   0ch,    name_fat32l
                        ptype   0eh,    name_fat16l
                        ptype   0fh,    name_extended
                        ptype   11h,    name_hidden12
                        ptype   14h,    name_hidden16
                        ptype   16h,    name_hidden16
                        ptype   17h,    name_hidden7
                        ptype   1bh,    name_hidden32
                        ptype   1ch,    name_hidden32
                        ptype   1eh,    name_hidden16
                        ptype   40h,    name_venix
                        ptype   41h,    name_ppc
                        ptype   42h,    name_dynamic
                        ptype   52h,    name_cpm80
                        ptype   63h,    name_unix
                        ptype   64h,    name_netware
                        ptype   65h,    name_netware
                        ptype   82h,    name_swap
                        ptype   83h,    name_linux
                        ptype   85h,    name_extended
                        ptype   8eh,    name_lvm
                        ptype   0a5h,   name_freebsd
                        ptype   0a6h,   name_openbsd
                        ptype   0a9h,   name_netbsd
                        ptype   0dah,   name_nonfs
                        ptype   0dbh,   name_cpm86
                        ptype   0eeh,   name_gpt
                        ptype   0efh,   name_efi
                        ptype   0fdh,   name_raid
                        db      0

; Partition-type descriptions.
name_fat12              db      'DOS FAT12$'
name_fat16s             db      'DOS FAT16 (<32 MB)$'
name_fat16              db      'DOS FAT16$'
name_fat16l             db      'FAT16 (LBA)$'
name_fat32              db      'FAT32$'
name_fat32l             db      'FAT32 (LBA)$'
name_extended           db      'Extended (logical partitions omitted)$'
name_hpfs               db      'HPFS / NTFS / exFAT$'
name_hidden12           db      'Hidden FAT12$'
name_hidden16           db      'Hidden FAT16$'
name_hidden7            db      'Hidden HPFS / NTFS$'
name_hidden32           db      'Hidden FAT32$'
name_venix              db      'Venix 80286$'
name_ppc                db      'PowerPC PReP$'
name_dynamic            db      'Windows dynamic / legacy type 42$'
name_cpm80              db      'CP/M-80$'
name_unix               db      'Unix / GNU Hurd / SCO$'
name_netware            db      'Novell NetWare$'
name_swap               db      'Linux swap / Solaris$'
name_linux              db      'Linux filesystem$'
name_lvm                db      'Linux LVM$'
name_freebsd            db      'FreeBSD / BSD$'
name_openbsd            db      'OpenBSD$'
name_netbsd             db      'NetBSD$'
name_nonfs              db      'Non-filesystem data$'
name_cpm86              db      'Digital Research CP/M-86 / Concurrent CP/M$'
name_gpt                db      'GPT protective container$'
name_efi                db      'EFI system partition$'
name_raid               db      'Linux RAID$'
name_unknown            db      'Unknown$'

; Buffers and private stack.
                        even
mbr                     db      512 dup (0)
dma_space               db      1024 dup (0)
; Materialized storage: both loaders allocate it even without BSS metadata.
stack_space             db      1024 dup (0)
stack_end               label   word

start:
                        cli
                        mov     ax,cs
                        mov     ss,ax
                        mov     sp,offset stack_end
                        sti
                        cld
                        call    detect_os
                        push    cs
                        pop     ds
                        push    cs
                        pop     es
                        mov     os_kind,al
                        call    select_backend
                        call    arguments
; A 512-byte-aligned physical DMA address cannot straddle a 64K boundary.
; Round the start of our 1024-byte reserve upwards, independent of load CS.
                        mov     ax,offset dma_space+15
                        mov     cl,4
                        shr     ax,cl
                        mov     bx,cs
                        add     ax,bx
                        add     ax,31
                        and     ax,0ffe0h
                        mov     dma_seg,ax
                        cmp     os_kind,OS_CCPM
                        jne     single_user
                        call    concurrent_init
                        call    take_disk
single_user:
                        mov     ah,2
                        call    sector_io
                        jnc     mbr_read
                        mov     dx,offset err_read
                        jmp     near ptr failure
mbr_read:
                        mov     ax,dma_seg
                        mov     ds,ax
                        xor     si,si
                        push    cs
                        pop     es
                        mov     di,offset mbr
                        mov     cx,256
                        rep     movsw
                        push    cs
                        pop     ds
                        cmp     word ptr mbr+510,0aa55h
                        je      signature_ok
                        mov     dx,offset err_signature
                        jmp     near ptr failure
signature_ok:
                        cmp     chosen,0
                        jne     activate
                        call    release_disk
                        call    listing
                        jmp     near ptr success
activate:
; Validate the complete table before changing any bytes. Do not repair
; corrupt boot indicators or accept overlapping/wrapping primary extents.
                        call    validate_table
                        xor     ax,ax
                        mov     al,chosen
                        dec     ax
                        mov     cl,4
                        shl     ax,cl
                        mov     si,offset mbr+446
                        add     si,ax
                        mov     al,[si+4]
                        or      al,al
                        jnz     selected_present
                        mov     dx,offset err_empty
                        jmp     near ptr parameter_failure
selected_present:
                        call    is_container
                        jnc     selected_normal
                        mov     dx,offset err_container
                        jmp     near ptr parameter_failure
selected_normal:
; GPT protective and dynamic-disk containers are unsafe activation targets.
                        cmp     al,0eeh
                        je      selected_container
                        cmp     al,42h
                        jne     selected_usable
selected_container:
                        mov     dx,offset err_container
                        jmp     near ptr parameter_failure
selected_usable:
                        cmp     byte ptr [si],80h
                        jne     modify_flags
                        mov     dx,offset msg_already
                        call    puts
                        jmp     near ptr activation_done
modify_flags:
                        mov     byte ptr mbr+446,0
                        mov     byte ptr mbr+462,0
                        mov     byte ptr mbr+478,0
                        mov     byte ptr mbr+494,0
                        mov     byte ptr [si],80h
; Copy only our modified MBR to the aligned DMA buffer.
                        mov     es,dma_seg
                        mov     si,offset mbr
                        xor     di,di
                        mov     cx,256
                        rep     movsw
                        mov     ah,3
                        call    sector_io
                        jnc     write_done
                        mov     dx,offset err_write
                        jmp     near ptr failure
write_done:
                        mov     ah,2
                        call    sector_io
                        jnc     verify_read
                        mov     dx,offset err_verify_read
                        jmp     near ptr failure
verify_read:
                        mov     es,dma_seg
                        mov     si,offset mbr
                        xor     di,di
                        mov     cx,256
                        repe    cmpsw
                        je      verified
                        mov     dx,offset err_verify
                        jmp     near ptr failure
verified:
                        mov     dx,offset msg_activated
                        call    puts
activation_done:
                        call    release_disk
                        cmp     reboot,0
                        je      success
                        call    flush_os
                        mov     dx,offset msg_reboot
                        call    puts
; IBM warm reset through the processor reset vector, not a CP/M warm start.
                        cli
                        mov     ax,40h
                        mov     es,ax
                        mov     word ptr es:[72h],1234h
                        db      0eah
                        dw      0,0ffffh
success:
                        xor     al,al
                        jmp     near ptr terminate
parameter_failure:
                        mov     show_usage,1
failure:
                        push    dx
                        call    release_disk
                        pop     dx
                        call    puts
                        cmp     show_usage,0
                        je      failure_exit
                        call    usage
failure_exit:
                        mov     al,1
terminate:
                        cmp     os_kind,OS_DOS
                        jne     cpm_exit
; INT 20h also supports PC-DOS 1.x. CS still addresses our PSP.
; No DOS ERRORLEVEL is returned (CP/M has no equivalent here either).
                        int     20h
cpm_exit:
                        xor     dx,dx
                        xor     cx,cx
                        call    cpm_call
                        jmp     near ptr cpm_exit

; ----------------------------------------------------------------------
; Launch detection, following HELLO. Entry DS must still be unchanged.
; Returns AL = OS_DOS or OS_CPM; a failed assumption traps through INT 2h.
; Records the BDOS compatibility version for the separate backend choice.
; ----------------------------------------------------------------------
detect_os:
                        mov     ax,ds
                        mov     cx,cs
                        cmp     ax,cx
                        jne     unknown_os

; DOS COM: a PSP with INT 20h at offset 0.
                        cmp     word ptr ds:[0],20cdh
                        jne     check_cpm
                        mov     al,OS_DOS
                        ret

; CP/M-86 8080 model: flag +5 and code-group segment +3.
check_cpm:
                        cmp     byte ptr ds:[5],1
                        jne     unknown_os
                        cmp     word ptr ds:[3],cx
                        jne     unknown_os
                        push    es
                        xor     ax,ax
                        mov     es,ax
                        mov     ax,es:[380h]
                        or      ax,es:[382h]
                        pop     es
                        jz      unknown_os
                        mov     cl,12
                        call    cpm_call
                        mov     cs:cpm_version,ax
                        mov     al,OS_CPM
                        ret
unknown_os:
                        int     2
                        jmp     short unknown_os
; ----------------------------------------------------------------------
; Launch personality and disk backend are separate decisions. These are
; the existing CP/M-86 1.1 and CCP/M-86 3.1 IBM PC backend requirements.
; ----------------------------------------------------------------------
select_backend:
                        cmp     os_kind,OS_DOS
                        je      backend_ready
                        mov     ax,cpm_version
                        cmp     ax,22h
                        je      backend_ready
                        cmp     ax,1022h
                        je      backend_ready
                        and     ah,0fh
                        cmp     ah,4
                        je      probe_concurrent
                        cmp     ah,6
                        jne     backend_unknown
probe_concurrent:
                        mov     cl,163
                        call    cpm_call
                        cmp     ax,1431h
                        jne     backend_unknown
                        mov     os_kind,OS_CCPM
backend_ready:
                        ret
backend_unknown:
                        jmp     near ptr unknown_os
; Bounded command-tail parser. FCBs are deliberately ignored: CP/M can
; interpret a drive token while constructing its default FCBs.
arguments:
                        xor     cx,cx
                        mov     cl,byte ptr ds:[80h]
                        cmp     cx,127
                        jbe     arg_branch_1
                        jmp     near ptr bad_arguments
arg_branch_1:
                        mov     si,81h
next_token:
                        jcxz    arg_branch_2
                        jmp     short arg_branch_2_skip
arg_branch_2:
                        jmp     near ptr arguments_done
arg_branch_2_skip:
                        lodsb
                        dec     cx
                        cmp     al,' '
                        jne     arg_branch_3
                        jmp     near ptr next_token
arg_branch_3:
                        cmp     al,9
                        jne     arg_branch_4
                        jmp     near ptr next_token
arg_branch_4:
                        cmp     al,'/'
                        jne     arg_branch_5
                        jmp     near ptr reboot_token
arg_branch_5:
                        cmp     al,'['
                        jne     arg_branch_6
                        jmp     near ptr reboot_token
arg_branch_6:
                        cmp     al,'1'
                        jae     arg_branch_7
                        jmp     near ptr bad_arguments
arg_branch_7:
                        cmp     al,'9'
                        jbe     arg_branch_8
                        jmp     near ptr bad_arguments
arg_branch_8:
                        xor     bx,bx
number_digit:
                        sub     al,'0'
                        xor     ah,ah
                        push    ax
                        mov     ax,bx
                        mov     dx,10
                        mul     dx
                        mov     bx,ax
                        pop     ax
                        add     bx,ax
                        cmp     bx,128
                        jbe     arg_branch_9
                        jmp     near ptr bad_arguments
arg_branch_9:
                        jcxz    arg_branch_10
                        jmp     short arg_branch_10_skip
arg_branch_10:
                        jmp     near ptr partition_token
arg_branch_10_skip:
                        mov     al,[si]
                        cmp     al,':'
                        jne     arg_branch_11
                        jmp     near ptr disk_token
arg_branch_11:
                        cmp     al,'0'
                        jae     arg_branch_12
                        jmp     near ptr partition_token
arg_branch_12:
                        cmp     al,'9'
                        jbe     arg_branch_13
                        jmp     near ptr partition_token
arg_branch_13:
                        inc     si
                        dec     cx
                        jmp     near ptr number_digit
disk_token:
                        cmp     disk_seen,0
                        je      arg_branch_14
                        jmp     near ptr bad_arguments
arg_branch_14:
                        mov     disk_seen,1
                        inc     si
                        dec     cx
                        dec     bx
                        mov     disk_unit,bl
                        jmp     near ptr token_end
partition_token:
                        cmp     bx,4
                        jbe     arg_branch_15
                        jmp     near ptr bad_arguments
arg_branch_15:
                        cmp     chosen,0
                        je      arg_branch_16
                        jmp     near ptr bad_arguments
arg_branch_16:
                        mov     chosen,bl
                        jmp     near ptr token_end
reboot_token:
                        mov     bl,al
                        cmp     reboot,0
                        je      arg_branch_17
                        jmp     near ptr bad_arguments
arg_branch_17:
                        jcxz    arg_branch_18
                        jmp     short arg_branch_18_skip
arg_branch_18:
                        jmp     near ptr bad_arguments
arg_branch_18_skip:
                        lodsb
                        dec     cx
                        and     al,0dfh
                        cmp     al,'R'
                        je      arg_branch_19
                        jmp     near ptr bad_arguments
arg_branch_19:
                        mov     reboot,1
                        jcxz    arg_branch_20
                        jmp     short arg_branch_20_skip
arg_branch_20:
                        jmp     near ptr arguments_done
arg_branch_20_skip:
                        cmp     bl,'['
                        jne     token_end
                        cmp     byte ptr [si],']'
                        je      arg_branch_21
                        jmp     near ptr token_end
arg_branch_21:
                        inc     si
                        dec     cx
token_end:
                        jcxz    arg_branch_22
                        jmp     short arg_branch_22_skip
arg_branch_22:
                        jmp     near ptr arguments_done
arg_branch_22_skip:
                        mov     al,[si]
                        cmp     al,' '
                        jne     arg_branch_23
                        jmp     near ptr next_token
arg_branch_23:
                        cmp     al,9
                        jne     arg_branch_24
                        jmp     near ptr next_token
arg_branch_24:
bad_arguments:
                        mov     show_usage,1
                        mov     dx,offset err_args
                        jmp     near ptr failure
arguments_done:
                        cmp     reboot,0
                        jne     arg_branch_25
                        jmp     near ptr arguments_valid
arg_branch_25:
                        cmp     chosen,0
                        jne     arg_branch_26
                        jmp     near ptr bad_arguments
arg_branch_26:
arguments_valid:
; Disk failures show usage when a disk was explicitly specified.
                        mov     al,disk_seen
                        mov     show_usage,al
                        ret
; Carry set for extended partition types. The entries are displayed,
; but neither activated nor followed into EBRs.
is_container:
                        cmp     al,5
                        je      container_yes
                        cmp     al,0fh
                        je      container_yes
                        cmp     al,85h
                        je      container_yes
                        clc
                        ret
container_yes:
                        stc
                        ret
validate_table:
                        mov     si,offset mbr+446
                        mov     bp,4
                        xor     bx,bx
validate_entry:
                        mov     al,[si]
                        cmp     al,80h
                        jne     table_branch_27
                        jmp     near ptr count_active
table_branch_27:
                        or      al,al
                        je      table_branch_28
                        jmp     near ptr invalid_table
table_branch_28:
                        jmp     near ptr flag_valid
count_active:
                        inc     bx
                        cmp     bx,1
                        jbe     table_branch_29
                        jmp     near ptr invalid_table
table_branch_29:
flag_valid:
                        cmp     byte ptr [si+4],0
                        je      table_branch_30
                        jmp     near ptr check_extent
table_branch_30:
; An empty slot may not be marked active.
                        cmp     byte ptr [si],0
                        je      table_branch_31
                        jmp     near ptr invalid_table
table_branch_31:
                        jmp     near ptr validate_next
check_extent:
                        mov     ax,[si+8]
                        mov     dx,[si+10]
                        mov     cx,ax
                        or      cx,dx
                        jnz     table_branch_32
                        jmp     near ptr invalid_table
table_branch_32:
                        mov     cx,[si+12]
                        or      cx,[si+14]
                        jnz     table_branch_33
                        jmp     near ptr invalid_table
table_branch_33:
                        add     ax,[si+12]
                        adc     dx,[si+14]
                        jnc     table_branch_34
                        jmp     near ptr invalid_table
table_branch_34:
; Save exclusive end, then compare every later occupied primary slot.
                        push    bx
                        mov     di,si
                        mov     cx,bp
                        dec     cx
                        jcxz    table_branch_35
                        jmp     short table_branch_35_skip
table_branch_35:
                        jmp     near ptr extent_checked
table_branch_35_skip:
compare_extent:
                        add     di,16
                        cmp     byte ptr [di+4],0
                        jne     table_branch_36
                        jmp     near ptr compare_next
table_branch_36:
; If later.start >= current.end, they are disjoint.
                        cmp     word ptr [di+10],dx
                        jbe     table_branch_37
                        jmp     near ptr compare_next
table_branch_37:
                        jae     table_branch_38
                        jmp     near ptr test_other_end
table_branch_38:
                        cmp     word ptr [di+8],ax
                        jb      table_branch_39
                        jmp     near ptr compare_next
table_branch_39:
test_other_end:
                        push    ax
                        push    dx
                        mov     ax,[di+8]
                        mov     dx,[di+10]
                        add     ax,[di+12]
                        adc     dx,[di+14]
                        jnc     table_branch_40
                        jmp     near ptr overlap_found
table_branch_40:
; If later.end <= current.start, they are disjoint.
                        cmp     dx,[si+10]
                        jae     table_branch_41
                        jmp     near ptr no_overlap
table_branch_41:
                        jbe     table_branch_42
                        jmp     near ptr overlap_found
table_branch_42:
                        cmp     ax,[si+8]
                        ja      table_branch_43
                        jmp     near ptr no_overlap
table_branch_43:
overlap_found:
                        pop     dx
                        pop     ax
                        pop     bx
                        jmp     near ptr invalid_table
no_overlap:
                        pop     dx
                        pop     ax
compare_next:
                        loop    compare_extent
extent_checked:
                        pop     bx
validate_next:
                        add     si,16
                        dec     bp
                        jz      table_branch_44
                        jmp     near ptr validate_entry
table_branch_44:
                        ret
invalid_table:
                        mov     dx,offset err_table
                        jmp     near ptr failure
; Display primary slots only. Raw IDs and exact LBA/sector counts remain
; useful even for unknown or nonbootable filesystems; no bootability claim.
listing:
                        mov     dx,offset msg_disk
                        call    puts
                        xor     ax,ax
                        mov     al,disk_unit
                        inc     ax
                        xor     dx,dx
                        call    decimal32
                        mov     dx,offset msg_columns
                        call    puts
                        mov     si,offset mbr+446
                        mov     bp,1
                        mov     listed,0
list_entry:
                        cmp     byte ptr [si+4],0
                        je      list_next
                        mov     listed,1
                        mov     dx,bp
                        add     dl,'0'
                        call    putch
                        mov     dx,offset msg_space
                        call    puts
                        mov     dl,' '
                        cmp     byte ptr [si],80h
                        jne     not_active
                        mov     dl,'*'
not_active:
                        call    putch
                        mov     dx,offset msg_space
                        call    puts
                        mov     al,[si+4]
                        call    hexbyte
                        mov     dx,offset msg_space
                        call    puts
                        mov     ax,[si+8]
                        mov     dx,[si+10]
                        call    decimal32
                        mov     dx,offset msg_space
                        call    puts
                        mov     ax,[si+12]
                        mov     dx,[si+14]
                        call    decimal32
                        mov     dx,offset msg_space
                        call    puts
                        mov     al,[si+4]
                        call    type_name
                        call    puts
                        mov     dx,offset newline
                        call    puts
list_next:
                        add     si,16
                        inc     bp
                        cmp     bp,5
                        jne     list_entry
                        cmp     listed,0
                        jne     list_finished
                        mov     dx,offset msg_no_parts
                        call    puts
list_finished:
                        mov     dx,offset msg_legend
                        call    puts
                        ret
type_name:
                        mov     type_id,al
                        push    si
                        mov     si,offset types
find_type:
                        lodsb
                        or      al,al
                        jz      unknown_type
                        mov     bx,[si]
                        add     si,2
                        cmp     al,type_id
                        jne     find_type
                        mov     dx,bx
                        jmp     short type_return
unknown_type:
                        mov     dx,offset name_unknown
type_return:
                        pop     si
                        ret
; Only ES is saved by the common BDOS entry. Callers save live values
; where needed; pointer-returning calls are consumed at their call site.
cpm_call:
                        push    es
                        int     0e0h
                        pop     es
                        ret
; Prints DS:DX. No caller needs AX, BX, CX or DX preserved here.
puts:
                        cmp     os_kind,OS_DOS
                        jne     puts_cpm
                        mov     ah,9
                        int     21h
                        ret
puts_cpm:
                        mov     cl,9
                        call    cpm_call
                        ret
; AX holds the other hex digit; CX holds the decimal output count.
putch:
                        push    ax
                        push    cx
                        cmp     os_kind,OS_DOS
                        jne     putch_cpm
                        mov     ah,2
                        int     21h
                        jmp     short putch_done
putch_cpm:
                        mov     cl,2
                        call    cpm_call
putch_done:
                        pop     cx
                        pop     ax
                        ret
hexbyte:
                        mov     ah,al
                        mov     cl,4
                        shr     al,cl
                        call    hex_digit
                        mov     al,ah
                        and     al,0fh
                        call    hex_digit
                        ret
hex_digit:
                        mov     dl,al
                        add     dl,'0'
                        cmp     dl,'9'
                        jbe     digit_ready
                        add     dl,7
digit_ready:
                        call    putch
                        ret
; Unsigned 32-bit decimal DX:AX; 8086 two-step division avoids overflow.
decimal32:
                        push    si
                        mov     bx,10
                        xor     cx,cx
divide_decimal:
                        mov     si,ax
                        mov     ax,dx
                        xor     dx,dx
                        div     bx
                        xchg    ax,si
                        div     bx
                        push    dx
                        inc     cx
                        mov     dx,si
                        mov     si,ax
                        or      si,dx
                        jnz     divide_decimal
print_decimal:
                        pop     dx
                        add     dl,'0'
                        call    putch
                        loop    print_decimal
                        pop     si
                        ret
usage:
                        mov     dx,offset usage_dos
                        cmp     os_kind,OS_DOS
                        je      usage_print
                        mov     dx,offset usage_cpm
usage_print:
                        call    puts
                        ret
flush_os:
                        cmp     os_kind,OS_DOS
                        jne     flush_cpm
                        mov     ah,0dh
                        int     21h
                        ret
flush_cpm:
                        mov     cl,13
                        call    cpm_call
                        ret
; BIOS registers are rebuilt for each attempt. The saved vector expects
; an interrupt frame; PUSHF/CALL FAR provides it, including IF clear.
; Returns CF and AH. Callers reload their transfer cursors afterwards;
; AX, BX, CX, DX and ES are scratch. DS retains the program segment.
sector_io:
                        mov     io_function,ah
                        mov     tries,3
io_attempt:
                        mov     ah,io_function
                        mov     al,1
                        mov     cx,1
                        xor     dh,dh
                        mov     dl,disk_unit
                        add     dl,80h
                        mov     es,dma_seg
                        xor     bx,bx
                        call    bios_call
                        jnc     io_ok
                        mov     io_error,ah
                        xor     ah,ah
                        mov     dl,disk_unit
                        add     dl,80h
                        call    bios_call
                        dec     tries
                        jnz     io_attempt
                        mov     ah,io_error
                        stc
                        jmp     short io_return
io_ok:
                        xor     ah,ah
                        clc
io_return:
                        cld
                        ret
bios_call:
                        cmp     os_kind,OS_CCPM
                        je      saved_bios
                        int     13h
                        ret
saved_bios:
                        pushf
                        cli
                        call    dword ptr bios_vector
                        ret
; Obtain and validate the particular CCP/M 3.1 IBM PC interface. No
; application data transfers through XIOS's partition-relative read/write.
concurrent_init:
; Function 154 returns SYSDAT in ES. Consume it before restoring ES.
                        mov     cl,154
                        push    es
                        int     0e0h
                        mov     sys_seg,es
                        pop     es
                        mov     cl,156
                        call    cpm_call
                        mov     pd_off,bx
                        mov     es,sys_seg
                        mov     ax,es:[bx+10h]
                        mov     uda_seg,ax
                        mov     ax,es:[28h]
                        mov     word ptr xios_vector,ax
                        mov     ax,es:[2ah]
                        mov     word ptr xios_vector+2,ax
                        mov     cl,153
                        call    cpm_call
                        cmp     ax,4
                        jb      local_console
                        jmp     near ptr concurrent_busy
local_console:
                        mov     bx,2ch
                        mul     bx
                        mov     es,sys_seg
                        add     ax,es:[54h]
                        mov     vc_off,ax
                        mov     bx,ax
                        cmp     word ptr es:[bx+0eh],0
                        je      get_equipment
                        jmp     near ptr concurrent_busy
get_equipment:
                        xor     cx,cx
                        call    xios_pointer
                        mov     es,sys_seg
                        mov     bx,ax
                        xor     ax,ax
                        mov     al,disk_unit
                        cmp     ax,es:[bx+2]
                        jb      get_bios_vector
                        mov     dx,offset err_disk
                        jmp     near ptr parameter_failure
get_bios_vector:
                        mov     cx,1
                        call    xios_pointer
                        mov     bx,ax
                        mov     es,sys_seg
                        mov     ax,es:[bx]
                        mov     word ptr bios_vector,ax
                        mov     ax,es:[bx+2]
                        mov     word ptr bios_vector+2,ax
                        or      ax,word ptr bios_vector
                        jnz     concurrent_ready
                        jmp     near ptr unknown_os
concurrent_ready:
                        ret
xios_pointer:
                        push    ds
                        mov     ds,sys_seg
                        mov     es,cs:uda_seg
                        mov     al,14
                        call    dword ptr cs:xios_vector
                        pop     ds
                        cld
                        ret
take_disk:
                        mov     es,sys_seg
                        cmp     word ptr es:[88h],0
                        je      reserve_process
                        jmp     near ptr concurrent_busy
reserve_process:
                        mov     bx,pd_off
                        mov     ax,es:[bx+6]
                        and     ax,2
                        mov     saved_keep,ax
                        or      word ptr es:[bx+6],2
                        mov     al,es:[bx+5]
                        mov     saved_priority,al
                        mov     process_reserved,1
                        mov     cl,145
                        mov     dl,150
                        call    cpm_call
                        mov     es,sys_seg
                        mov     bx,vc_off
                        pushf
                        cli
                        cmp     word ptr es:[bx+0eh],0
                        je      reserve_console
                        popf
                        jmp     near ptr concurrent_busy
reserve_console:
                        or      word ptr es:[bx+0eh],8
                        mov     console_reserved,1
                        popf
                        mov     dx,offset queue_block
                        mov     cl,135
                        call    cpm_call
                        or      ax,ax
                        jnz     queue_failed
                        mov     dx,offset queue_block
                        mov     cl,137
                        call    cpm_call
                        or      ax,ax
                        jnz     queue_failed
                        mov     disk_reserved,1
; No disk owner can change the filesystem state after this recheck.
                        mov     es,sys_seg
                        cmp     word ptr es:[88h],0
                        jne     concurrent_busy
                        ret
queue_failed:
                        mov     dx,offset err_queue
                        jmp     near ptr failure
concurrent_busy:
                        mov     dx,offset err_busy
                        jmp     near ptr failure
release_disk:
                        cmp     os_kind,OS_CCPM
                        jne     released
                        cmp     disk_reserved,0
                        je      release_console
                        mov     dx,offset queue_block
                        mov     cl,139
                        call    cpm_call
                        or      ax,ax
                        jnz     release_failed
                        mov     disk_reserved,0
release_console:
                        cmp     console_reserved,0
                        je      release_process
                        mov     es,sys_seg
                        mov     bx,vc_off
                        pushf
                        cli
                        and     word ptr es:[bx+0eh],0fff7h
                        popf
                        mov     console_reserved,0
release_process:
                        cmp     process_reserved,0
                        je      released
                        mov     es,sys_seg
                        mov     bx,pd_off
                        and     word ptr es:[bx+6],0fffdh
                        mov     ax,saved_keep
                        or      es:[bx+6],ax
                        mov     dl,saved_priority
                        mov     cl,145
                        call    cpm_call
                        mov     process_reserved,0
released:
                        ret
release_failed:
; Do not claim success or reboot if releasing mutual exclusion failed.
                        mov     disk_reserved,0
                        call    release_disk
                        mov     dx,offset err_release
                        call    puts
                        mov     al,1
                        jmp     near ptr terminate
program                 ends
                        end     entry
