# edlin for CP/M-86

A port of **IBM PC-DOS 1.1 EDLIN** to **CP/M-86 1.1**, assembled with MASM.
The DOS source it starts from, a MASM 1.10 reconstruction of `EDLIN.COM`
byte-for-byte identical to the original, is in
[tsupplis/pcdos11-hacking](https://github.com/tsupplis/pcdos11-hacking).

The aim is to show that a real DOS 1.x utility can be taken back to CP/M-86
by changing only what the operating system change breaks: system calls,
startup, memory sizing, register usage and file I/O. The editing algorithms
are the original ones, unchanged.

The port runs on a real CP/M-86 1.1 (tested in PCE, IBM PC/XT model) and
under the `cpm86` emulator from
[cpm86-crossdev](https://github.com/tsupplis/cpm86-crossdev).

| File | What |
|---|---|
| [edlin.asm](edlin.asm) | The CP/M-86 port. Every change carries a `CPM86 PORT` comment. |
| [Makefile](Makefile) | Builds `edlin.cmd`. |
| [edlin-cpm86-plan.md](edlin-cpm86-plan.md) | The plan the port was done from. |

---

## Building

| Tool | Role |
|---|---|
| `pcdev_masm` (MASM 5.10) | Assemble `edlin.asm` |
| `pcdev_link` (LINK 3.65a) | Link to `edlin.exe` |
| `pcdev_exe2bin` | Flatten the EXE to a binary image starting at 100h |
| `bin2cmd` | Wrap the image in a CP/M-86 CMD header (8080 model) |
| `cmdinfo` | Show the CMD group descriptors |

```
make clean && make
```

```
pcdev_masm    edlin.asm         -> edlin.obj
pcdev_link    edlin.obj         -> edlin.exe   ("no stack segment" warning is expected)
pcdev_exe2bin edlin.exe         -> edlin.bin
bin2cmd -m 10000 edlin.bin      -> edlin.cmd
```

`cmdinfo edlin.cmd` must show `MAX(64.0k=65536)` for the code group (see
[Memory size](#6-memory-size-psp06h-means-nothing)).

## Running

```
A>EDLIN [d:]filename.ext
```

On the host, with the cpm86-crossdev tools:

```
cpm86 edlin.cmd myfile.txt
```

Or copy `edlin.cmd` to a CP/M-86 disk image with cpmtools
(`cpmcp -f ibmpc-514ss cpmtest.img edlin.cmd 0:`) and run it in PCE.

The editor behaves as on DOS. The file is read into memory (up to 3/4 of the
buffer), edits go to the in-memory text, and `E` writes `name.$$$`, renames the
old file to `name.BAK` and the `$$$` file to `name`.

### Commands

```
[n]                    Edit line n (default: the line after the current one)
[n][,m]L               List lines (default: 23 lines around the current line)
[n][,m]D               Delete lines
[n]I                   Insert lines before line n (end with ^Z on a line of its own)
[n][,m][?]Sstr         Search for str; ? asks "O.K.?" at each match
[n][,m][?]Rstr^Zrepl   Replace str by repl; ? asks "O.K.?" at each change
[n]A                   Append n lines from the file (fill to 3/4 of memory by default)
[n]W                   Write n lines to the output file (frees memory for A)
E                      End: save, keep the old file as .BAK
Q                      Quit without saving (asks "Abort edit (Y/N)?")
```

`n` and `m` are line numbers, `.` is the current line and `#` is the line after
the last one.

---

## Memory layout

CP/M-86 loads `edlin.cmd` as an **8080 model** program: one group holding code,
data and stack, with CS = DS = ES and the base page at offset 0. That is the
same picture as a DOS COM file, which is why the source needs no segment
changes.

| Range | Contents |
|---|---|
| 0000h-00FFh | Base page: FCB 1 at 5Ch, FCB 2 at 6Ch, command tail at 80h |
| 0100h-0ABFh | Code |
| 0AC0h-0BD9h | `$`-terminated messages (end of the CMD image) |
| 0BDAh-10A8h | Uninitialised work area: FCB 2 (the `$$$` file), variables, input buffers, the 2 x 128 byte record buffers, 256 byte stack |
| 10A9h-0FFFFh | Text buffer (lines ending in CR LF, then a ^Z) |

The DOS original had the work area at 0A58h-0D48h and the text at 0D49h. The
addresses in the `edlin.asm` comments are those of the port.

---

## What had to change

The changes below are in the order you meet them when porting. Most of them
apply to any MASM DOS 1.x program going to CP/M-86.

### 1. System call interface: `int 21h` becomes `int 0E0h` with `CL`

CP/M-86 BDOS is `int 0E0h`. The function number goes in **CL**, the parameter
in **DX** (or DL), and the result comes back in **AL** (bytes) or **AX = BX**
(words). For the FCB and console calls edlin uses, the function numbers match
DOS:

| Function | DOS `AH` | BDOS `CL` | Notes |
|---|---|---|---|
| Console input with echo | 01h | 01h | |
| Console output | 02h | 02h | char in DL |
| Print `$` string | 09h | 09h | |
| Buffered input | 0Ah | 0Ah | same buffer layout, no DOS template keys |
| Open / close file | 0Fh / 10h | 0Fh / 10h | AL = 0FFh if not found |
| Delete file | 13h | 13h | |
| Create file | 16h | 16h | |
| Rename file | 17h | 17h | new name at FCB+16 in both |
| Set DMA (DTA) offset | 1Ah | 1Ah | |
| Set DMA segment | - | 33h | CP/M-86 only |
| Random block read | **27h** | - | **CP/M fn 27h is something else**, see 5 |
| Random block write | **28h** | - | **CP/M fn 28h is something else**, see 5 |
| Terminate (`int 20h`) | - | 00h | `CL = 0`, `DL = 0` |

`int 20h` becomes:

```asm
xor     cx, cx          ; BDOS fn 0: system reset, back to the CCP
mov     dl, 0           ; DL = 0: release the program's memory
int     0E0h
```

### 2. BDOS keeps far fewer registers than DOS

This caused most of the port's bugs. DOS `int 21h` returns every register
except AX (and the documented result registers) unchanged. Code written for
DOS 1.x, edlin included, freely keeps live values in BX, CX, DX, SI, DI, BP
and ES across calls.

CP/M-86 BDOS only returns AL / AX = BX. **Treat AX, BX, CX, DX, SI, DI, BP
and ES as destroyed by every call.** DS has to be right on entry, and the
port reloads it anyway.

edlin does it in three ways:

- **A `bdos` macro** replaces every `int 21h`. After the call it rebuilds DS
  and ES from CS. That is safe here because the program is a single group.
  ES matters because `movs`, `cmps`, `scas` and `stos` all write or compare
  through ES:DI:

  ```asm
  bdos            macro
                  int     0E0h
                  push    cs
                  pop     ds
                  push    cs
                  pop     es
                  endm
  ```

- **`print_char` saves everything** (AX, BX, CX, DX, SI, DI, BP) around its
  BDOS call. Every byte of output goes through it, so all the loops that print
  while holding state in registers are covered at once.

- **The remaining call sites** save what is live (`push`/`pop`) or reload it
  after the call.

Where edlin keeps values live across BDOS calls, and what goes wrong when
the BDOS changes them:

| Register | Where it was live | What happened |
|---|---|---|
| ES | `rep movsb` building FCB 2 after the open call | the name was copied into another segment; `create` then saw an uninitialised FCB: *"CP/M Error On _: Invalid Drive, BDOS Function = 22 File = (garbage)"* |
| DX | `create` reused DX = FCB 2 left by `delete`; `Q` reused it from `close` to `delete`; disk-full `close` | BDOS called on a garbage FCB address |
| AX | `print_char` restored AX with `xchg ax, dx` after the call | the caller got BDOS's DX back in AL |
| BX | `conv10` keeps the leading-zero flag in BX while printing digits | leading zeros printed |
| DI | `print_line` counts down in DI | listing stopped after one character |
| SI | the line editor keeps the line pointer in SI across buffered input | wrong line edited |
| BP | `I` keeps the end of the insertion gap in BP across the line-number print | the text was copied back with a garbage length (a 441 byte file became 51 KB) |

### 3. Stack: set SS:SP yourself, and make it bigger

A DOS COM program starts with SS = CS. Under CP/M-86, don't rely on the
inherited SS:SP: it may be the CCP's stack. The port sets SS and SP at
`_start`, before the first BDOS call:

```asm
_start: mov     ax, cs
        mov     ds, ax
        mov     es, ax
        mov     ss, ax
        mov     sp, offset stack_top
```

DOS switched to its own stack inside `int 21h`, so edlin got by with 40
bytes. With the register saves around the BDOS calls, the interrupt frames
and nested calls, that is too small. `stack_size` is now 256 bytes.

### 4. Set the DMA segment

BDOS fn 1Ah sets only the offset of the DMA address; fn 33h sets its segment.
CP/M-86 starts a program with the DMA segment at the base page, which for an
8080 model program is already CS. The port still sets it explicitly once, at
`init`:

```asm
mov     cl, 33h         ; set DMA segment
mov     dx, cs
bdos
```

Every record transfer now goes through the port's own 128 byte buffers (see
5), so the DMA *offset* is set right before each record call.

### 5. No byte-level block I/O: DOS fn 27h / 28h

edlin does all file I/O with DOS **random block read / write (fn 27h / 28h)**
and a **record size of 1**. Each call moves any number of bytes. The random
record field of the FCB is a byte offset, and the program rewinds it after a
read by however many bytes it decides not to keep:

```asm
sub     ds:[fcb1_rr], di            ; DI = bytes read but not kept
sbb     word ptr ds:[fcb1_rr + 2], 0
```

CP/M-86 has none of that:

- Function numbers 27h and 28h exist, but they are *Get Allocation Vector* and
  *Write Protect Disk*. Calling them "for I/O" fails with no error.
- File I/O happens in **128 byte records only**: fn 21h / 22h (read / write
  random) move one record at the random record number in FCB bytes 33-35.
- There is no record size field. FCB byte 14 is the BDOS's private `s2`
  byte, so writing 1 or 128 there as DOS did corrupts the FCB.

Rounding byte counts up to whole records is **not** a fix:

- Writes put the garbage after the text into the file, and the next `W`
  starts on a fresh record, leaving a hole in the middle of the file.
- Reads that fill memory round up past 0FFFFh and wrap into the base page.
- Rewinding the read position by whole records loses or repeats bytes.

The port keeps the original byte-oriented logic and puts a small layer under
it that behaves exactly like DOS fn 27h / 28h with a record size of 1:

| Routine | Replaces | Does |
|---|---|---|
| `blk_read` | fn 27h on FCB 1 | Reads CX bytes to DI starting at the byte offset `rd_pos`. It reads the record `rd_pos / 128` into `rd_buf` and copies from `rd_pos mod 128`, as many times as needed. It returns CX = bytes read and AL = 1 when the end of file is hit, and advances `rd_pos`. |
| `blk_write` | fn 28h on FCB 2 | Appends CX bytes from DX to `wr_buf` and writes a record each time `wr_buf` is full. AL is non-zero on disk full. |
| `blk_wr_eof` | the final 1 byte ^Z write in `E` | Adds a ^Z, pads the last record with ^Z and writes it. |
| `rec_io` | | One fn 21h / 22h call: sets the DMA to the buffer, makes the call, keeps every register but AX. |

Both routines keep all registers except their results, as DOS did. The
call sites are therefore the original ones, with `call blk_read` /
`call blk_write` in place of `int 21h`. The rewind after a partial read is the
original two instructions, now applied to `rd_pos`.

### 6. Memory size: PSP:06h means nothing

On DOS, the word at PSP:06h is the number of bytes available in the segment,
and edlin sizes its text buffer from it. The CP/M-86 base page has a group
length at 06h (a 24 bit value, part of the group descriptors at 00h-11h),
which is not a free-memory count.

The port books the whole 64 KB group in the CMD header (`bin2cmd -m 10000`
sets the code group `MAX` to 64 KB) and uses a constant:

```asm
seg_max         equ     0FFFFh          ; last usable address
```

### 7. FCBs: same offsets, different meaning past byte 11

| Offset | DOS | CP/M-86 |
|---|---|---|
| 0 | drive (0 = default) | drive (0 = default) |
| 1-8 | name | name |
| 9-11 | extension | extension (high bits = attributes) |
| 12-13 | current block | `ex`, `s1` |
| 14-15 | **record size** | `s2`, `rc` (BDOS private) |
| 16-31 | file size, date, ... (rename: new name at 16) | allocation map (rename: new name at 16) |
| 32 | current record | `cr` current record |
| 33-36 | random record (4 bytes when record size < 64) | random record `r0 r1 r2` (3 bytes) |

What that means for the port:

- **Don't write a record size.** The writes to FCB+14 are gone.
- **Clear bytes 12-35 of any FCB you build.** FCB 2 lives in uninitialised
  memory past the end of the image. DOS ignored the leftovers there; CP/M
  doesn't. The port zeroes them before the `delete` and `create` calls.
- FCB 1 and FCB 2 in the base page (5Ch, 6Ch) and the command tail at 80h are
  where DOS has them, and the CCP parses the file name the same way.

### 8. Startup: no drive check in AL

DOS enters a program with AL = 0FFh if the drive letter in FCB 1 is invalid,
and edlin tests it. CP/M passes no such flag, so the port clears AL before
the test. A bad drive is reported by the BDOS itself when the file is opened.

### 9. ^Z and record padding

CP/M text files are a whole number of records long. The text ends at the first
^Z and the rest of the last record is padding. When the first read finds a
^Z, the port stops the text before it and marks the file as fully read, so
`A` and `E` don't go on reading the padding. The output file always ends with
a ^Z, with the last record padded with ^Z.

### 10. No ^C / ^Break handler

edlin installs an `int 23h` handler (DOS fn 25h) for ^Break at the prompt and
during `I`. CP/M-86 has no equivalent, so those two calls are removed. The
handlers (`break_cmd`, `break_ins`) are left in place but are never reached.
^C at the start of a line of input makes the BDOS reboot to the CCP.

### 11. MASM practicalities

- `int 21h` sites became longer (`bdos` macro, saves, reloads), and a few
  `jz short` / `jnz short` went out of range. They became a short
  conditional jump around a near `jmp`, as in `quit_cmd`.
- `make` runs `unix2dos` on the source first: MASM 5.10 needs CR LF.
- The MASM tools run under `emu2`, which wants a terminal. In a
  non-interactive shell, run `script -q /dev/null make`.

---

## Change summary by routine

| Routine | Change |
|---|---|
| equates | `bdos_*` function numbers; `seg_max`; `stack_size` 256; work area grows by `rd_pos`, `wr_cnt`, `rd_buf`, `wr_buf` |
| `bdos` macro | new: `int 0E0h`, then DS = ES = CS |
| `_start` | DS, ES, SS = CS; SP = `stack_top` |
| `init` | set DMA segment; clear AL for the drive test |
| `create_tmp` | zero FCB 2 bytes 12-35; reload DX for `create` |
| `init_buf` | no record size; `rd_pos` / `wr_cnt` reset; `seg_max`; `blk_read`; ^Z handling |
| `command`, `insert_cmd` | `int 23h` installs removed; registers saved around buffered input |
| `append_cmd` | `blk_read`; rewind on `rd_pos` |
| `write_recs` | `blk_write`; reload DX for `close` on disk full |
| `edit_cmd` | SI saved around buffered input |
| `quit_cmd` | reload DX for `delete` |
| `exit_cmd` | `blk_wr_eof` for the final ^Z; reload DX for `close` |
| `print_char` | saves AX, BX, CX, DX, SI, DI, BP around the BDOS call |
| `blk_read`, `blk_write`, `blk_wr_eof`, `wr_flush`, `rec_io` | new |
| every `int 21h` / `int 20h` | `bdos` with CL = function |

---

## Known limitations

- **No template editing keys.** On DOS, buffered input (fn 0Ah) keeps the
  previous line as a template: F1 / F3 copy it, F5 makes a new one. That is
  how a line is edited with the `[n]` command. CP/M-86 fn 0Ah has no template,
  so an edited line has to be typed again in full; pressing just Enter keeps
  it.
- **^C reboots.** The temporary `name.$$$` is left behind and unsaved edits
  are lost. Leave with `E` or `Q`.
- **64 KB is assumed.** `seg_max` takes the whole 64 KB group booked in the
  CMD header for granted. On a system with less free memory than that, the
  loader can grant less than `MAX` (the minimum is the 3 KB image) and the
  text buffer would run past the end of the allocation. A sturdier version
  would take the real group length from base page 00h-02h.
- **Files larger than memory** work as on DOS, with `W` and `A`; `E` copies
  the rest of the input file through memory.
- **Output size.** The saved file always ends with a ^Z and is padded to a
  record boundary. A file whose text ends exactly on a record boundary gains
  one record of ^Z.
