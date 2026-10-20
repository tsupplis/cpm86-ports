; --------------------------------------------------------------------
; SETBOOT -- primary MBR partition selector, IBM PC, 8086/8088.
; MASM 1.10. Same payload in DOS COM and CP/M-86 8080-model CMD.
; Self-contained source: no INCLUDE files or external object modules.
; Reference disassemblies and disk-command sources are not build inputs.
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

; Main flow:
;   start -> detect_os -> select_backend -> arguments
;         -> (CCP/M: concurrent_init -> take_disk) -> read MBR
;         -> listing, or validate -> change flags -> write -> verify
;         -> release_disk -> optional reboot -> terminate
;
; All routines are near labels in one segment. DS normally addresses
; program data; ES is temporary for BIOS DMA and CCP/M system pointers.
; detect_os must inspect the unmodified entry DS before normalization.
;
; MBR: 512 bytes, partition table at 446 (01BEh), signature at 510.
; Each of four 16-byte entries contains:
;   +0 boot flag (00h or 80h)       +4 partition type (00h = empty)
;   +1..3 start CHS, +5..7 end CHS  +8 start LBA, +12 sector count
; LBA and count are little-endian 32-bit values. CHS, bootstrap code,
; disk signature and every other MBR byte must remain unchanged.
;
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
; launch interface / selected backend (OS_* constants)
os_kind                 db      OS_DOS
; raw BDOS function 12 result for backend selection
cpm_version             dw      0
; 0 = list only; otherwise primary slot 1..4
chosen                  db      0
; 1 = reboot after successful or already-active selection
reboot                  db      0
; zero-based physical disk; BIOS drive = 80h + disk_unit
disk_unit               db      0
; 1 = an explicit disk: token was supplied
disk_seen               db      0
; print usage after a parameter-related error
show_usage              db      0
; 1 after displaying at least one occupied primary entry
listed                  db      0
type_id                 db      0
tries                   db      0
; transfer request retained across resets/retries
io_function             db      0
; last BIOS failure status, returned in AH
io_error                db      0
; aligned transfer buffer, addressed as ES:0000
dma_seg                 dw      0

; CCP/M pointers and acquired-resource state.
; CCP/M system data segment
sys_seg                 dw      0
; process descriptor offset in sys_seg
pd_off                  dw      0
; current process user-data-area segment
uda_seg                 dw      0
; local virtual-console control block offset in sys_seg
vc_off                  dw      0
; XIOS far entry obtained from SYSDAT:0028h
xios_vector             dd      0
; original INT 13h far entry from XIOS setup data
bios_vector             dd      0
saved_keep              dw      0
saved_priority          db      0
; restore P_KEEP and priority only if acquired
process_reserved        db      0
; clear NoSwitch only if this command set it
console_reserved        db      0
; return the MXdisk token only if this command owns it
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
; working copy, later the expected image for verification
mbr                     db      512 dup (0)
; room to round up to a 512-byte physical boundary
dma_space               db      1024 dup (0)
; Materialized storage: both loaders allocate it even without BSS metadata.
; implicitly used below stack_end; this storage is required
stack_space             db      1024 dup (0)
stack_end               label   word


; ----------------------------------------------------------------------
; Install the private stack first. Leave DS unchanged until detect_os
; checks the loader's entry value. Only then set DS and ES to our segment.
; ----------------------------------------------------------------------
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

; ----------------------------------------------------------------------
; Validate every entry before any write, even for an already-active slot.
; Select the requested entry with (slot - 1) * 16; numbers always refer to
; the original primary slots, including empty slots.
; ----------------------------------------------------------------------
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

; ----------------------------------------------------------------------
; The only intended disk changes are these four boot-indicator bytes.
; Clear all four, then mark the selected one 80h. Keep the complete expected
; sector in mbr so verification can detect changes outside the table too.
; ----------------------------------------------------------------------
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

; ----------------------------------------------------------------------
; Read back into the DMA buffer, leaving mbr as the expected image.
; Compare all 256 words below. A read failure or mismatch goes to failure
; and cannot reach the reboot path.
; ----------------------------------------------------------------------
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

; ----------------------------------------------------------------------
; The write is verified, or the selected entry was already active.
; Release CCP/M restrictions before flushing and before a possible reset.
; A release error terminates inside release_disk instead of returning here.
; ----------------------------------------------------------------------
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
                        jmp     near ptr terminate
parameter_failure:
                        mov     show_usage,1

; ----------------------------------------------------------------------
; Common recoverable-error exit. DX points to a $-terminated message.
; Save DX across cleanup because it is still needed by puts. Acquisition
; flags make release_disk safe even after partial CCP/M initialization.
; ----------------------------------------------------------------------
failure:
                        push    dx
                        call    release_disk
                        pop     dx
                        call    puts
                        cmp     show_usage,0
                        je      failure_exit
                        call    usage
failure_exit:
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

; ----------------------------------------------------------------------
; Parse the length-bounded tail at DS:0080h, not the default FCBs.
; SI = next byte; CX = remaining bytes; BX = accumulated decimal number.
; A trailing colon makes a number a disk selector; a bare number is a slot.
; Tokens may be reordered, but duplicates and attached suffixes are errors.
;
; 8086 conditional jumps and JCXZ have short displacements only. The
; conditional + JMP NEAR sequences bridge longer distances deliberately;
; MASM 1.10 does not automatically expand these branches.
; ----------------------------------------------------------------------
arguments:
                        xor     cx,cx
                        mov     cl,byte ptr ds:[80h]
                        cmp     cx,127
                        jbe     tail_length_ok
                        jmp     near ptr bad_arguments
tail_length_ok:
                        mov     si,81h
next_token:
                        jcxz    args_at_end
                        jmp     short args_have_byte
args_at_end:
                        jmp     near ptr arguments_done
args_have_byte:
                        lodsb
                        dec     cx
                        cmp     al,' '
                        jne     args_not_space
                        jmp     near ptr next_token
args_not_space:
                        cmp     al,9
                        jne     args_not_tab
                        jmp     near ptr next_token
args_not_tab:
                        cmp     al,'/'
                        jne     args_not_slash
                        jmp     near ptr reboot_token
args_not_slash:
                        cmp     al,'['
                        jne     args_not_bracket
                        jmp     near ptr reboot_token
args_not_bracket:
                        cmp     al,'1'
                        jae     args_digit_lower_ok
                        jmp     near ptr bad_arguments
args_digit_lower_ok:
                        cmp     al,'9'
                        jbe     args_digit_upper_ok
                        jmp     near ptr bad_arguments
args_digit_upper_ok:
                        xor     bx,bx

; ----------------------------------------------------------------------
; AL is the current digit. Accumulate BX = BX * 10 + digit.
; The previous value is at most 128, so multiplication cannot overflow.
; Checking each digit rejects long numeric tokens without a wider counter.
; ----------------------------------------------------------------------
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
                        jbe     number_within_range
                        jmp     near ptr bad_arguments
number_within_range:
                        jcxz    number_at_end
                        jmp     short number_has_next
number_at_end:
                        jmp     near ptr partition_token
number_has_next:
                        mov     al,[si]
                        cmp     al,':'
                        jne     number_not_drive
                        jmp     near ptr disk_token
number_not_drive:
                        cmp     al,'0'
                        jae     number_digit_lower_ok
                        jmp     near ptr partition_token
number_digit_lower_ok:
                        cmp     al,'9'
                        jbe     number_digit_upper_ok
                        jmp     near ptr partition_token
number_digit_upper_ok:
                        inc     si
                        dec     cx
                        jmp     near ptr number_digit
disk_token:
                        cmp     disk_seen,0
                        je      disk_not_repeated
                        jmp     near ptr bad_arguments
disk_not_repeated:
                        mov     disk_seen,1
                        inc     si
                        dec     cx
                        dec     bx
                        mov     disk_unit,bl
                        jmp     near ptr token_end
partition_token:
                        cmp     bx,4
                        jbe     partition_in_range
                        jmp     near ptr bad_arguments
partition_in_range:
                        cmp     chosen,0
                        je      partition_not_repeated
                        jmp     near ptr bad_arguments
partition_not_repeated:
                        mov     chosen,bl
                        jmp     near ptr token_end

; ----------------------------------------------------------------------
; Remember '/' or '[' in BL. Accept /R, [R or [R], ignoring letter case.
; Only '[' permits the optional closing bracket. token_end rejects any
; other suffix, and arguments_done requires a partition with reboot.
; ----------------------------------------------------------------------
reboot_token:
                        mov     bl,al
                        cmp     reboot,0
                        je      reboot_not_repeated
                        jmp     near ptr bad_arguments
reboot_not_repeated:
                        jcxz    reboot_missing_letter
                        jmp     short reboot_has_letter
reboot_missing_letter:
                        jmp     near ptr bad_arguments
reboot_has_letter:
                        lodsb
                        dec     cx
                        and     al,0dfh
                        cmp     al,'R'
                        je      reboot_letter_valid
                        jmp     near ptr bad_arguments
reboot_letter_valid:
                        mov     reboot,1
                        jcxz    reboot_at_end
                        jmp     short reboot_has_next
reboot_at_end:
                        jmp     near ptr arguments_done
reboot_has_next:
                        cmp     bl,'['
                        jne     token_end
                        cmp     byte ptr [si],']'
                        je      reboot_close_bracket
                        jmp     near ptr token_end
reboot_close_bracket:
                        inc     si
                        dec     cx
token_end:
                        jcxz    token_at_end
                        jmp     short token_has_next
token_at_end:
                        jmp     near ptr arguments_done
token_has_next:
                        mov     al,[si]
                        cmp     al,' '
                        jne     token_not_space
                        jmp     near ptr next_token
token_not_space:
                        cmp     al,9
                        jne     token_not_tab
                        jmp     near ptr next_token
token_not_tab:
bad_arguments:
                        mov     show_usage,1
                        mov     dx,offset err_args
                        jmp     near ptr failure
arguments_done:
                        cmp     reboot,0
                        jne     reboot_needs_partition
                        jmp     near ptr arguments_valid
reboot_needs_partition:
                        cmp     chosen,0
                        jne     reboot_partition_valid
                        jmp     near ptr bad_arguments
reboot_partition_valid:
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

; ----------------------------------------------------------------------
; Inspect, but never modify, all four primary entries.
; SI = current entry; BP = entries remaining; BX = active-entry count.
; Allow flags 00h/80h, at most one active entry, nonzero starts/sizes,
; nonwrapping 32-bit extents and no overlap between occupied primary slots.
; Return on success; a malformed table jumps to the common failure exit.
; ----------------------------------------------------------------------
validate_table:
                        mov     si,offset mbr+446
                        mov     bp,4
                        xor     bx,bx
validate_entry:
                        mov     al,[si]
                        cmp     al,80h
                        jne     check_inactive_flag
                        jmp     near ptr count_active
check_inactive_flag:
                        or      al,al
                        je      inactive_flag_valid
                        jmp     near ptr invalid_table
inactive_flag_valid:
                        jmp     near ptr flag_valid
count_active:
                        inc     bx
                        cmp     bx,1
                        jbe     active_count_valid
                        jmp     near ptr invalid_table
active_count_valid:
flag_valid:
                        cmp     byte ptr [si+4],0
                        je      check_empty_flag
                        jmp     near ptr check_extent
check_empty_flag:
; An empty slot may not be marked active.
                        cmp     byte ptr [si],0
                        je      empty_flag_valid
                        jmp     near ptr invalid_table
empty_flag_valid:
                        jmp     near ptr validate_next
check_extent:
                        mov     ax,[si+8]
                        mov     dx,[si+10]
                        mov     cx,ax
                        or      cx,dx
                        jnz     extent_start_nonzero
                        jmp     near ptr invalid_table
extent_start_nonzero:
                        mov     cx,[si+12]
                        or      cx,[si+14]
                        jnz     extent_size_nonzero
                        jmp     near ptr invalid_table
extent_size_nonzero:
                        add     ax,[si+12]
                        adc     dx,[si+14]
                        jnc     extent_does_not_wrap
                        jmp     near ptr invalid_table
extent_does_not_wrap:
; Save exclusive end, then compare every later occupied primary slot.
                        push    bx
                        mov     di,si
                        mov     cx,bp
                        dec     cx
                        jcxz    no_later_entries
                        jmp     short later_entries_remain
no_later_entries:
                        jmp     near ptr extent_checked
later_entries_remain:

; ----------------------------------------------------------------------
; DX:AX holds current.start + current.size, the exclusive extent end.
; DI walks later occupied entries; compare each pair only once.
; Disjoint means later.start >= current.end OR later.end <= current.start.
; Equality permits adjacent partitions. Save the outer active count in BX.
; ----------------------------------------------------------------------
compare_extent:
                        add     di,16
                        cmp     byte ptr [di+4],0
                        jne     compare_occupied_entry
                        jmp     near ptr compare_next
compare_occupied_entry:
; If later.start >= current.end, they are disjoint.
                        cmp     word ptr [di+10],dx
                        jbe     compare_start_high
                        jmp     near ptr compare_next
compare_start_high:
                        jae     compare_start_low
                        jmp     near ptr test_other_end
compare_start_low:
                        cmp     word ptr [di+8],ax
                        jb      compare_other_end
                        jmp     near ptr compare_next
compare_other_end:
test_other_end:
                        push    ax
                        push    dx
                        mov     ax,[di+8]
                        mov     dx,[di+10]
                        add     ax,[di+12]
                        adc     dx,[di+14]
                        jnc     other_end_does_not_wrap
                        jmp     near ptr overlap_found
other_end_does_not_wrap:
; If later.end <= current.start, they are disjoint.
                        cmp     dx,[si+10]
                        jae     compare_end_high
                        jmp     near ptr no_overlap
compare_end_high:
                        jbe     compare_end_low
                        jmp     near ptr overlap_found
compare_end_low:
                        cmp     ax,[si+8]
                        ja      entries_overlap
                        jmp     near ptr no_overlap
entries_overlap:

; ----------------------------------------------------------------------
; Balance the saved extent-end words and active count before failure;
; common cleanup must not inherit the inner comparison loop's stack.
; ----------------------------------------------------------------------
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
                        jz      all_entries_checked
                        jmp     near ptr validate_entry
all_entries_checked:
                        ret
invalid_table:
                        mov     dx,offset err_table
                        jmp     near ptr failure
; Display primary slots only. Raw IDs and exact LBA/sector counts remain
; useful even for unknown or nonbootable filesystems; no bootability claim.

; ----------------------------------------------------------------------
; Walk the four primary entries with SI; BP is the original slot 1..4.
; Skip empty entries without renumbering later ones. Mark flag 80h with '*'.
; Print raw hexadecimal type plus a name when known; names do not establish
; bootability. LBA and length are exact counts of 512-byte sectors.
; ----------------------------------------------------------------------
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

; ----------------------------------------------------------------------
; Advance for every slot, including empty ones. listed controls the
; explicit message when no occupied primary entry was encountered.
; ----------------------------------------------------------------------
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

; ----------------------------------------------------------------------
; Input AL = partition type. Return DS:DX -> $-terminated description.
; Search (byte type, word string offset) rows until the zero sentinel.
; Preserve SI, the caller's partition cursor; AX, BX and DX are scratch.
; ----------------------------------------------------------------------
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

; ----------------------------------------------------------------------
; Print AL as two uppercase hexadecimal digits. AX/CX/DX are scratch.
; AH keeps the original byte across the first digit; putch preserves AX.
; ----------------------------------------------------------------------
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

; ----------------------------------------------------------------------
; Print unsigned DX:AX, including zero and FFFFFFFFh.
; Divide the high word by ten, then remainder:low by ten, yielding a full
; 32-bit quotient using only 8086 DIV. Push remainders and pop them in
; print order; CX counts digits. Preserve the caller's partition cursor SI.
; ----------------------------------------------------------------------
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

; ----------------------------------------------------------------------
; Display the convention for this launch interface: /R on DOS, [R] on
; CP/M. The parser accepts all three reboot spellings on either system.
; ----------------------------------------------------------------------
usage:
                        mov     dx,offset usage_dos
                        cmp     os_kind,OS_DOS
                        je      usage_print
                        mov     dx,offset usage_cpm
usage_print:
                        call    puts
                        ret

; ----------------------------------------------------------------------
; Run the OS disk-reset/flush service after releasing MXdisk, so BDOS
; can obtain its own disk exclusion before the optional machine reset.
; ----------------------------------------------------------------------
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

; ----------------------------------------------------------------------
; Transfer one sector at cylinder 0, head 0, sector 1: the physical MBR.
; Input AH = 02h read / 03h write; disk_unit and dma_seg select disk/buffer.
; Rebuild BIOS registers on every attempt. Retry at most three times,
; resetting the controller after failure. Return CF and last AH status.
; Callers reload numeric registers and cursors after this routine.
; ----------------------------------------------------------------------
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

; ----------------------------------------------------------------------
; DOS and single-user CP/M use INT 13h. CCP/M calls its saved BIOS entry
; only while take_disk's process/console/disk restrictions are held.
; PUSHF + CALL FAR supplies the interrupt return frame expected by IRET;
; CLI clears IF for entry while the pushed flags keep the caller's IF.
; ----------------------------------------------------------------------
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

; ----------------------------------------------------------------------
; Read CCP/M-86 3.1 IBM PC pointers before acquiring any resources.
; BDOS 154 returns SYSDAT in ES; 156 returns the process descriptor in BX;
; 153 returns the console number. Follow SYSDAT to XIOS and console data.
; XIOS 14/CX=0 supplies equipment data (hard-disk count at +2).
; XIOS 14/CX=1 supplies setup data (original INT 13h vector at +0).
; Ordinary XIOS reads are partition-relative and cannot address the MBR.
; ----------------------------------------------------------------------
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

; ----------------------------------------------------------------------
; Input CX = selector; return AX = offset inside sys_seg.
; XIOS needs DS=SYSDAT and ES=UDA. Restore DS on return. Its callers reload
; ES immediately, so saving ES here would serve no caller requirement.
; ----------------------------------------------------------------------
xios_pointer:
                        push    ds
                        mov     ds,sys_seg
                        mov     es,cs:uda_seg
                        mov     al,14
                        call    dword ptr cs:xios_vector
                        pop     ds
                        cld
                        ret

; ----------------------------------------------------------------------
; Acquire CCP/M protections in order: process, console, then MXdisk.
; Remember original P_KEEP/priority; set P_KEEP and request priority 150.
; Require an unbuffered local console and no open files. Recheck open files
; after acquiring MXdisk to close the race with another disk user.
; Set each *_reserved flag only after acquiring that resource, allowing
; a shared release_disk cleanup after any subsequent error.
; ----------------------------------------------------------------------
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

; ----------------------------------------------------------------------
; No-op outside CCP/M. Return the MXdisk token, clear our NoSwitch bit,
; then restore the original P_KEEP bit and priority. Acquisition flags guard
; each step, making partial setup safe. No media-cache invalidation is
; needed here: only MBR active flags have changed.
; ----------------------------------------------------------------------
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

; ----------------------------------------------------------------------
; Do not retry a failed queue-token return recursively. Clear the local
; retry flag, release console/process restrictions, report, and terminate.
; This path never returns to activation_done's optional reboot.
; ----------------------------------------------------------------------
release_failed:
; Do not claim success or reboot if releasing mutual exclusion failed.
                        mov     disk_reserved,0
                        call    release_disk
                        mov     dx,offset err_release
                        call    puts
                        jmp     near ptr terminate
program                 ends
                        end     entry
