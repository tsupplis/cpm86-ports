# edlin CP/M-86 Port

A port of **IBM PC-DOS 1.1 EDLIN** to **CP/M-86 1.11**, demonstrating that a
real DOS utility can be brought back to CP/M-86 with targeted, mechanical source
changes and no algorithmic rewrites.

---

## Overview

`edlin` is the IBM PC's original line editor, shipped with PC-DOS 1.0 and 1.1.
The source (`edlin.asm`) is a MASM 1.10 reconstruction of the PC-DOS 1.1 binary,
byte-for-byte identical to the original COM image.  This port changes only what
the DOS→CP/M-86 transition breaks — system calls, startup, memory ceiling, and
register preservation — leaving all editing algorithms untouched.

### Build requirements

| Tool | Role |
|---|---|
| `pcdev_masm` (MASM 5.10) | Assemble `edlin.asm` |
| `pcdev_link` (LINK 3.65a) | Link to `.exe` |
| `pcdev_exe2bin` | Strip the EXE header to a flat binary |
| `bin2cmd` | Wrap the binary in a CP/M-86 CMD header |
| `cmdinfo` | Verify the CMD segment descriptor |
| `emu2` | Run CP/M-86 CMD files on the host |

### Build

```
make clean && make
```

### Run

```
make run FILE=myfile.txt
```

or directly:

```
emu2 edlin.cmd myfile.txt
```

### Command reference

```
[n]              Edit line n (default: line after current)
[n][,m]L         List lines (default: 23 lines around current)
[n][,m]D         Delete lines
[n]I             Insert lines before n  (end with ^Z or ^Break)
[n][,m][?]Sstr   Search for str; ? prompts "O.K.?" at each match
[n][,m][?]Rstr^Zrepl   Replace str with repl
[n]A             Append n lines from file (fill memory if no n)
[n]W             Write n lines to disk (frees memory for A)
E                End edit: save, rename old file to .BAK
Q                Quit without saving (asks "Abort edit (Y/N)?")
```

---

## Toolchain notes

**Why MASM instead of RASM-86?**  The source is MASM 1.10 format (macros,
`assume`, segment directives, `proc`/`endp`).  Keeping MASM preserves the
direct correspondence between the DOS original and the CP/M-86 port, making
diffs trivial to audit.

**The build pipeline:**

```
pcdev_masm  edlin.asm  →  edlin.obj
pcdev_link  edlin.obj  →  edlin.exe   (linker warning "no stack segment" is expected)
pcdev_exe2bin edlin.exe → edlin.bin   (strip 512-byte EXE header)
bin2cmd -m 10000 edlin.bin → edlin.cmd
```

**Why `-m 10000`?**  CP/M-86 does not grant a program its full 64 KB
automatically.  The `MAX` field in the CMD code-segment descriptor tells the
loader how much memory to allocate.  `-m 10000` sets `MAX = 0FFFFh` (64 KB),
giving edlin the full segment for its text buffer.  Without this flag, the
loader allocates only the bytes actually present in the binary (~2.8 KB), and
the text buffer would be zero bytes.

`cmdinfo edlin.cmd` should report `MAX(64.0k=65536)`.

---

## DOS → CP/M-86 porting recipe

The changes below are numbered rules reusable for any MASM DOS→CP/M-86 port.

### 1. Remove DOS interrupt-vector installs

`int 21h` with `AH=25h` (Set Interrupt Vector) has no CP/M-86 equivalent.
Drop the call entirely.  Under CP/M-86, `^C` triggers a warm boot; the custom
handler is never reached.

### 2. Remap `int 21h` → `int 0E0h`, `AH=fn` → `CL=fn`

CP/M-86 BDOS is called via `int 0E0h` with the function number in `CL`, not
`AH`.  FCB-based function numbers **01h–1Ah are numerically identical** between
DOS and CP/M-86 2.2 — only the interrupt and register change.

```asm
; DOS
mov  ah, 0Fh        ; open file
mov  dx, offset fcb
int  21h

; CP/M-86
mov  cl, 0Fh
mov  dx, offset fcb
int  0E0h
```

`int 20h` (terminate) becomes BDOS fn 0 with `DL=0`:

```asm
xor  cx, cx
mov  dl, 0
int  0E0h
```

### 3. fn 27h and fn 28h are a trap

In CP/M-86, function numbers 27h and 28h are **not** random-block I/O:

| Number | DOS | CP/M-86 |
|---|---|---|
| 27h | Random Block Read | `DRV_ALLOCVEC` (return allocation bitmap) |
| 28h | Random Block Write | `DRV_SETRO` (software write-protect drive) |

The correct CP/M-86 replacements are **fn 21h (`F_READRAND`)** and
**fn 22h (`F_WRITERAND`)**, which read/write one 128-byte record at the random
record counter (`fcb_rr`).  CP/M-86 has no multi-record bulk read; wrap the
call in a loop:

- Convert byte count to record count: `(bytes + 127) / 128`
- Loop calling fn 21h / fn 22h, advancing the DMA address by 128 after each
  call, incrementing `fcb_rr` by 1 after each successful read/write
- After a partial read (discarding some trailing bytes), back up `fcb_rr` by
  `ceil(unused_bytes / 128)` so the next read overlaps correctly

### 4. Set DS = ES = CS at entry

Even `org 100h` programs receive **DS = base-page segment, not CS**, on entry
under CP/M-86.  Add three instructions at the very top of the entry point:

```asm
_start:
    mov  ax, cs
    mov  ds, ax
    mov  es, ax
    jmp  short init
```

### 5. Set the DMA segment at startup

BDOS fn 1Ah (Set DMA Address) sets the **offset** only.  The DMA segment
defaults to the base-page segment, so file reads would land there instead of in
your program's data segment.  Call fn 33h once at startup with `DX = CS`:

```asm
mov  cl, 33h        ; BDOS fn 33h = Set DMA Segment
mov  dx, cs
int  0E0h
```

### 6. PSP offset 06h is not available memory

Under DOS, word `PSP:06h` holds the number of bytes available in the segment.
Under CP/M-86, the same offset holds the **length of the code group** in bytes
— not usable memory.  Do not read it as a memory ceiling.

Instead:

1. Use `bin2cmd -m 10000` to book 64 KB in the CMD header
2. Replace the runtime read with a compile-time equate:
   ```asm
   seg_max  equ  0FFFFh   ; full 64 KB — booked in CMD header via -m 10000
   ```

### 7. BDOS clobbers more registers than DOS

DOS `int 21h` preserves `BX, CX, SI, DI, BP, DS, ES` across most calls.
CP/M-86 BDOS `int 0E0h` clobbers **AX, BX, CX, DX, SI, DI, and ES**.

Protect any output helper that is called while other registers hold live data.
For edlin, wrapping `print_char` with `push/pop` of all BDOS-clobbered registers
covers every call site:

```asm
print_char  proc near
    push bx
    push cx             ; BDOS clobbers CL (used as function number)
    push si
    push di
    push es
    push dx
    xchg ax, dx         ; DL = character to print
    mov  cl, 02h        ; BDOS fn 02h: console output
    int  0E0h
    xchg ax, dx
    pop  dx
    pop  es
    pop  di
    pop  si
    pop  cx
    pop  bx
    ret
print_char  endp
```

---

## Zero-page / PSP compatibility

| What edlin accesses | DOS PSP offset | CP/M-86 Zero Page | Safe? |
|---|---|---|---|
| FCB 1 (filename from command line) | 5Ch | 5Ch | ✅ identical |
| FCB 2 | 6Ch | 6Ch | ✅ identical |
| Command tail (length + text) | 80h / 81h | 80h / 81h | ✅ identical |
| Available memory | 06h (bytes in segment) | 06h (code group length) | ❌ use `seg_max` equate |

---

## FCB compatibility

The FCB layout is identical between DOS and CP/M-86:

| Field | Offset |
|---|---|
| Drive byte | +0 |
| File name (8 chars, space-padded) | +1 |
| Extension (3 chars) | +9 |
| Record size word | +14 |
| New name (rename FCB) | +16 |
| Random record number (dword) | +33 |

`$`-terminated strings (BDOS fn 09h) are also identical.

---

## Known limitations

- **No F-key template recall.**  DOS BUFIN (CON driver) keeps a template of the
  previous input line; F1 copies it one character at a time, F3 copies the rest,
  F5 sets a new template.  This is what makes edlin's line editor `[n]` usable
  on DOS: editing a long line means replaying the old text up to the change and
  typing the correction.  CP/M-86 BDOS fn 0Ah has no such template — every
  input starts blank.  The editor still works (all commands function correctly),
  but re-typing an existing line from scratch is tedious.  This is an inherent
  CP/M-86 platform limitation, not a porting bug.

- **^C triggers a warm boot, not a graceful abort.**  On DOS, edlin installs a
  custom `int 23h` handler (`break_cmd` during the command prompt,  `break_ins`
  during insert mode) that rebuilds the segments and stack and returns cleanly to
  the `*` prompt.  CP/M-86 has no equivalent of `int 23h` — `^C` during BDOS
  input causes an immediate warm boot, discarding all unsaved edits and the
  temporary `.$$$ ` file.  The handler code remains in the binary as dead code.
  **Always use `E` to save or `Q` to quit; never hit `^C`.**
- **File size** is limited to the 64 KB segment.  Edlin's `A` (Append) and `W`
  (Write) commands manage this: write lines out with `W`, then read more with
  `A`.
- **Record granularity**: because CP/M-86 F_READRAND operates in 128-byte
  units, the file-position back-adjustment after a partial read is rounded up to
  the nearest record boundary.  Up to 127 bytes may be re-read on the next
  `A` command, which is harmless because `scan_eof` and line-boundary detection
  discard the overlap.
