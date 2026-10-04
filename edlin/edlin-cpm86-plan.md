# edlin CP/M-86 Port Plan

## Goal

Demonstrate that a real DOS 1.1 utility can be ported back to CP/M-86 using
MASM as the assembler, with minimal source changes.  Document the findings —
what changed, why, and what pitfalls to watch for — in `README.md` so this
serves as a reference for future DOS→CP/M-86 ports.

## Overview

Port the historical PC-DOS 1.1 `edlin.asm` to CP/M-86 1.11.
The source is MASM 1.10 format and **already builds** to `edlin.cmd` via
`pcdev_masm` + `pcdev_link` + `pcdev_exe2bin` + `bin2cmd`.

The emulator currently passes DOS `int 21h` through to DOS stubs, masking all
bugs.  The goal is a binary that works on real CP/M-86 or a strict emulator
with **BDOS-only** system calls.

**Minimal-change philosophy:** only fix what is broken by the DOS→CP/M-86
transition.  Do not refactor, rename, or reorganise anything else.

Five targeted fixes, in recommended execution order:

1. Remove DOS ^C vector install (2 trivial deletions)
2. Remap every `int 21h` / `int 20h` to `int 0E0h` with `CL=function`
3. Fix entry segment setup, DMA segment, and memory ceiling
4. Fix file I/O: DOS fn 27h/28h ≠ BDOS fn 27h/28h
5. Fix BDOS register preservation

---

## Zero-page / PSP layout

| What edlin accesses | DOS PSP offset | CP/M-86 Zero Page | Safe? |
|---|---|---|---|
| FCB 1 (filename from cmdline) | 5Ch | 5Ch | ✅ identical |
| FCB 2 | 6Ch | 6Ch | ✅ identical |
| Command tail 80h/81h | 80h/81h | 80h/81h | ✅ identical |
| Memory size | 06h = segment beyond TPA | 00h–02h = code group length | ❌ fix in ST3 |

---

## Sub-task 1 — Remove the DOS ^C vector install

**Status:** `[ ] pending`

### Intent
Two `dos_setvec_23` triplets issue `int 21h` with AX=2523h — there is no
CP/M-86 equivalent and the call will crash or corrupt state.  Deleting just
these 6 lines is the safest first change and has zero effect on everything else.
`break_cmd` / `break_ins` bodies stay as dead code.

### Todo List
- [ ] Delete 3 lines at `command` (~line 265): `mov ax, dos_setvec_23` /
      `mov dx, offset break_cmd` / `int 21h`.
- [ ] Delete 3 lines at `insert_cmd` (~line 1290): same triplet for `break_ins`.
- [ ] Comment out `dos_setvec_23 equ 2523h` and add note: "no CP/M-86 equivalent".
- [ ] Add comment above `break_cmd`: "; unreachable under CP/M-86".

### Relevant Context
- `edlin.asm` lines 265–267, 1290–1292: the two triplets
- `edlin.asm` line 121: `dos_setvec_23 equ 2523h`

---

## Sub-task 2 — Remap DOS API to CP/M-86 BDOS

**Status:** `[ ] pending`

### Intent
Every `int 21h` → `int 0E0h` with `CL=function` instead of `AH=function`.
All FCB-based functions (0Fh–1Ah) have the **same numbers** in CP/M-86 2.2,
so only the interrupt number and the register change.

**CRITICAL exception — fn 27h and 28h:**
- DOS fn 27h = Random Block Read (used by edlin for bulk file reads)
- DOS fn 28h = Random Block Write (used by edlin for bulk file writes)
- CP/M-86 fn 27h = `DRV_ALLOCVEC` (return allocation bitmap) — completely different
- CP/M-86 fn 28h = `DRV_SETRO` (software write-protect drive) — completely different

These two functions require a separate sub-task (ST4).  Do **not** change their
interrupt number here; leave them as `int 21h` temporarily with a comment, to be
fixed in ST4.

**Official BDOS call convention:**
```asm
MOV  DX, parameter   ; 16-bit parameter (or DL for byte params like fn 02h)
MOV  CL, function    ; function number in CL only
INT  0E0h
; 8-bit result in AL=BL; 16-bit result in AX=BX
```

**Termination:** `int 20h` → `xor cx, cx` / `mov dl, 0` / `int 0E0h`
(BDOS fn 0, DL=0 = free memory and return to CCP).

### DOS → BDOS function map (functions remapped in this sub-task)

| DOS AH= | Name | BDOS CL= | Notes |
|---|---|---|---|
| 01h | Keyboard input w/ echo | 01h | same number, same result in AL |
| 02h | Console output | 02h | char in DL — identical |
| 09h | Print `$`-string | 09h | identical |
| 0Ah | Buffered keyboard input | 0Ah | buffer layout identical |
| 0Fh | Open file (FCB) | 0Fh | identical |
| 10h | Close file | 10h | identical |
| 13h | Delete file | 13h | identical |
| 16h | Create file | 16h | identical |
| 17h | Rename file | 17h | identical |
| 1Ah | Set DMA offset | 1Ah | offset only — segment fixed in ST3 |
| 27h | Random block read | **skip — ST4** | different function in CP/M-86 |
| 28h | Random block write | **skip — ST4** | different function in CP/M-86 |
| `int 20h` | Terminate | fn 00h | `xor cx,cx` / `mov dl,0` / `int 0E0h` |

### Todo List
- [ ] In the equate block (lines 107–121): rename `dos_*` to `bdos_*`; keep
      `dos_rdblock`/`dos_wrblock` values at 27h/28h but add comment "DEFERRED ST4".
      Add `bdos_dma_seg equ 33h`.
- [ ] In `print_char` (~line 1469): `mov ah, dos_display` + `int 21h`
      → `mov cl, bdos_display` + `int 0E0h`.
      This single change fixes all ~15 character-output call sites routed through it.
- [ ] Change every remaining `mov ah, bdos_xxx` + `int 21h` to
      `mov cl, bdos_xxx` + `int 0E0h` — **except** the two `dos_rdblock` /
      `dos_wrblock` sites (leave those as `int 21h` with a "DEFERRED ST4" comment).
- [ ] Replace all three `int 20h` with `xor cx, cx` / `mov dl, 0` / `int 0E0h`.
- [ ] Update "DOS:" source comments to "BDOS:" at changed sites only.

### Relevant Context
- `edlin.asm` lines 107–121: equate block
- `edlin.asm` line ~1469: `print_char`
- `int  21h` sites: ~25 total; leave ~4 untouched (2× rdblock, 2× wrblock)
- `int 20h` sites: 3 (lines ~609, ~1398, ~1448)

---

## Sub-task 3 — Entry setup, DMA segment, memory ceiling

**Status:** `[ ] pending`

### Intent
Three small startup fixes, each independent.

**A — Segment registers.**
CP/M-86 enters an `org 100h` program with DS = base-page segment, not CS.
Add 3 instructions at `_start` before `jmp short init`.

**B — DMA segment.**
BDOS fn 1Ah (Set DMA) sets the offset only.  The DMA segment defaults to the
base-page segment, so all file I/O would land there instead of in our code/data
segment.  Call BDOS fn 33h once at program start with DX=CS.

**C — Memory ceiling (`mem_top`).**
The code reads `ds:[psp_memsize]` (PSP offset 06h) as a byte count for the
text buffer ceiling.  Under CP/M-86, offset 06h is the length of the data group
in bytes — not the available memory.  Reading it as a ceiling would give a
nonsense (small) value.

**Approach: use the CMD header `MAX` booking + a compile-time equate.**

1. Tell `bin2cmd` to book the full 64 KB segment via `-m 10000` in the Makefile.
   CP/M-86 will then allocate exactly 64 KB for the program.
2. Replace the runtime `psp_memsize` read with a **fixed compile-time equate**:
   ```asm
   seg_max  equ  0FFFFh   ; full 64 KB segment booked by CMD header
   ```
   And in `init_buf`, replace `mov cx, ds:[psp_memsize]` / `dec cx` with:
   ```asm
   mov  cx, seg_max
   ```
   Everything else (`mem_top`, `buf_1qtr`, `buf_3qtr` computation) stays unchanged.

This is the **minimal change**: one equate added, two source lines replaced,
one Makefile argument added.  No runtime BDOS call, no new code structure.

### Todo List
- [ ] At `_start` (line 137), insert before `jmp short init`:
      ```asm
      mov  ax, cs
      mov  ds, ax
      mov  es, ax
      ```
- [ ] At the top of `init` (line 157), before any data access, insert:
      ```asm
      mov  cl, 33h         ; BDOS fn 33h = Set DMA Segment
      mov  dx, cs
      int  0E0h
      ```
- [ ] Add equate near the other limits (line ~125):
      ```asm
      seg_max  equ  0FFFFh         ; Full 64 KB — booked in CMD header via -m 10000
      ```
- [ ] In `init_buf` (line ~224): replace
      `mov  cx, ds:[psp_memsize]` / `dec cx` with:
      ```asm
      mov  cx, seg_max
      ```
      Leave the `mov word ptr ds:[mem_top], cx` and all buf_1qtr/buf_3qtr
      computation below it **unchanged**.
- [ ] In the Makefile, change the `bin2cmd` invocation to:
      ```makefile
      $(BIN2CMD) -m 10000 edlin.bin $@
      ```
- [ ] Run `cmdinfo edlin.cmd` to verify the segment MAX field is 0FFFFh.
- [ ] Comment out (do not delete) `psp_memsize equ 06h` with a note explaining
      why it is not used under CP/M-86.

### Relevant Context
- `edlin.asm` line 137: `_start`
- `edlin.asm` line 157: `init`
- `edlin.asm` line 96: `psp_memsize equ 06h`
- `edlin.asm` lines 224–226: the 3 lines to replace
- `bin2cmd -m 10000` books 64 KB (0FFFFh bytes); CP/M-86 pre-allocates that segment
- `cmdinfo edlin.cmd` confirms the MAX field in the CMD header

---

## Sub-task 4 — File I/O: replace DOS fn 27h/28h

**Status:** `[ ] pending`

### Intent
DOS fn 27h (Random Block Read) and fn 28h (Random Block Write) are **not**
present in CP/M-86 2.2.  Those numbers map to completely different disk
utility calls (`DRV_ALLOCVEC` and `DRV_SETRO`).

The correct CP/M-86 equivalents for record I/O are:
- **BDOS fn 21h (`F_READRAND`)** — read the record at FCB random counter into DMA
- **BDOS fn 22h (`F_WRITERAND`)** — write DMA contents to record at FCB random counter

Edlin uses fn 27h/28h with `recsiz=1` for byte-level streaming I/O, advancing
the random record counter as a byte offset.  CP/M-86 fn 21h/22h work in
fixed **128-byte records**, with the random counter in units of 128 bytes.

**Approach (minimal change): keep the random-I/O model, fix the record size.**

Replace each `dos_rdblock` / `dos_wrblock` call with the CP/M-86 random I/O
equivalents, operating on 128-byte records:
- Set `recsiz = 128` (already the CP/M-86 default; makes intent explicit)
- Pass `CX = (byte_count + 127) / 128` as the **record count**
- After each partial read, the `fcb1_rr` back-adjustment (which subtracts
  *unused bytes*) must divide by 128 — change `sub ds:[fcb1_rr], di` to
  shift DI right 7 before subtracting
- The final ^Z write in `exit_cmd` writes 1 record (128 bytes); CP/M-86
  convention pads with ^Z — correct behaviour, no change needed

**^Z termination:** `scan_eof` already searches the data for a ^Z and truncates —
this is correct CP/M behaviour.  Leave it unchanged.

### Todo List
- [ ] In `init_buf`: change `inc ax` (sets recsiz=1) to `mov ax, 128`;
      apply to both `fcb1_recsiz` and `fcb2_recsiz` writes.
- [ ] Update equates: `dos_rdblock` → `bdos_rdrand equ 21h` (F_READRAND);
      `dos_wrblock` → `bdos_wrrand equ 22h` (F_WRITERAND).
- [ ] At each `dos_rdblock` call site (`init_buf` and `append_lp`):
      - Before the call, convert byte count CX to record count:
        ```asm
        add  cx, 127
        mov  cl, 7
        shr  cx, cl          ; CX = (bytes+127)/128 = record count
        ```
      - Change `mov ah, dos_rdblock` + `int 21h`
        → `mov cl, bdos_rdrand` + `int 0E0h`
- [ ] At each `dos_wrblock` call site (`write_recs` and `exit_cmd`):
      - Same byte→record conversion before the call
      - Change `mov ah, dos_wrblock` + `int 21h`
        → `mov cl, bdos_wrrand` + `int 0E0h`
- [ ] Fix `fcb1_rr` back-adjustment in `append_set_end` (~line 513):
      ```asm
      ; Before: sub  ds:[fcb1_rr], di   (di = unused bytes)
      ; After:
      mov  ax, di
      add  ax, 127
      mov  cl, 7
      shr  ax, cl            ; AX = unused records
      sub  ds:[fcb1_rr], ax
      sbb  word ptr ds:[fcb1_rr + 2], 0
      ```
- [ ] Verify `exit_cmd` final write: 1 record, DMA = `endtxt` (contains ^Z).
      CP/M-86 pads the 128-byte record with ^Z — correct.  Add a comment only.

### Relevant Context
- `edlin.asm` lines 211–242: `init_buf` — recsiz init + first rdblock
- `edlin.asm` lines 464–518: `append_lp` + `append_set_end` — rdblock + rr adjust
- `edlin.asm` lines 574–598: `write_recs` — wrblock
- `edlin.asm` lines 1413–1448: `exit_cmd` — final wrblock
- seasip.info fn 21h (CP/M-86): F_READRAND, 128-byte record at random counter
- seasip.info fn 22h (CP/M-86): F_WRITERAND, same
- seasip.info fn 36h (24h): F_RANDREC — not needed; we track rr manually

---

## Sub-task 5 — BDOS register preservation

**Status:** `[ ] pending`

### Intent
DOS `int 21h` preserves BX, CX, SI, DI, BP, DS, ES across the call (only AX
and sometimes DX are clobbered).  CP/M-86 BDOS `int 0E0h` clobbers
**AX, BX, CX, DX, SI, DI, and ES**.

The **minimal fix**: make `print_char` save/restore BX, SI, DI and ES around
the BDOS call.  This protects every caller without touching any other routine.
A secondary fix is needed in `append_lp` where CX is live across a `set_dta` call.

Key problem sites:
- **`conv10`** holds BX (leading-zero sentinel `10h`) across repeated calls to
  `print_char`.  BDOS zeroes BX → leading zeros print as `0` digits.
- **`print_line`** holds DI (line count) across each `print_char` call.
  BDOS zeroes DI → loop terminates after first character.
- **`append_lp`**: CX (free-memory byte count) is live across the `set_dta`
  BDOS call before the `push cx` that saves it for `scan_eof`.

### Todo List
- [ ] In `print_char` (lines ~1466–1474): add saves around the BDOS call:
      ```asm
      print_char  proc near
                  push    bx
                  push    si
                  push    di
                  push    es
                  push    dx
                  xchg    ax, dx          ; DL = character
                  mov     cl, bdos_display
                  int     0E0h
                  xchg    ax, dx          ; restore AX
                  pop     dx
                  pop     es
                  pop     di
                  pop     si
                  pop     bx
                  ret
      print_char  endp
      ```
      This is the only routine that needs changing for all output paths.
- [ ] In `append_lp` (~line 466): move `push cx` to **before**
      `mov ah, dos_set_dta` / `int 21h` (the set_dta call), not after it.
      The matching `pop cx` stays where it is.
- [ ] Verify `print_msg` (direct fn 09h call): DX is already the string address,
      no other live registers — safe without change.
- [ ] Verify `prompt_yesno` (direct fn 09h + fn 01h calls): no live registers
      carried across — safe without change.

### Relevant Context
- `edlin.asm` lines 1466–1474: `print_char`
- `edlin.asm` lines 693–738: `conv10` / `print_digits` — BX live
- `edlin.asm` lines 775–818: `print_line` — DI live
- `edlin.asm` lines 464–476: `append_lp` — CX live across set_dta

---

## Implementation Notes

### Execution order
1 → 2 → 3 → 5 → 4 → 6

Sub-tasks 1–3 can each be verified with a build + smoke test.
Sub-task 5 must come before sub-task 4 (register corruption would mask I/O bugs).
Sub-task 4 is the most complex and should be last.

### Makefile change (sub-task 3)
```makefile
edlin.cmd: edlin.exe
	$(EXE2BIN) $< edlin.bin
	$(BIN2CMD) -m 10000 edlin.bin $@
	$(CMDINFO) $@
```
`-m 10000` books 64 KB (0x10000 bytes) in the CMD header.
`cmdinfo edlin.cmd` should show `MAX=FFFFh` or equivalent in the code segment descriptor.

### What is NOT changed
- MASM syntax, macros, segment/assume — build toolchain stays as-is
- FCB field offsets (5Ch, 6Ch, +1, +9, +14, +16, +33) — identical to DOS
- `$`-terminated strings — same format for BDOS fn 09h
- All buffer layout, `org 100h`, stack equates — unchanged
- All command algorithms (A, W, I, D, L, S, R, E, Q)
- `scan_eof` — ^Z detection already correct for CP/M

### Smoke test after each sub-task
```
make clean && make
cmdinfo edlin.cmd
emu2 edlin.cmd testfile.txt
```
Minimum: `*` prompt, `I` inserts a line, `L` lists it, `E` saves.
Sub-task 4: edit a file > 256 bytes, verify content correct after `E`.

---

## Sub-task 6 — Write README.md

**Status:** `[ ] pending`

### Intent
Once all five port sub-tasks pass the smoke test, document the experience so
this project serves as a reusable reference for future DOS→CP/M-86 MASM ports.

### Todo List

- [ ] Create `README.md` with the following sections:

  **Overview**
  - What edlin is; why this port is interesting (real DOS 1.1 utility, minimal changes)
  - Build requirements: `pcdev_masm`, `pcdev_link`, `pcdev_exe2bin`, `bin2cmd`,
    `cmdinfo`, `emu2`
  - How to build: `make clean && make`
  - How to run: `emu2 edlin.cmd <filename>`
  - Brief edlin command reference (trim from the source file header)

  **Toolchain notes**
  - Why MASM is used instead of RASM-86 (preserves DOS source, easier reconciliation)
  - The `exe2bin` + `bin2cmd -m 10000` pipeline; what `cmdinfo` reports
  - Why `-m 10000` is needed: CP/M-86 does not auto-allocate 64 KB; segment booking
    in the CMD header is how the program reserves its full address space

  **DOS → CP/M-86 porting recipe** (the core reference section)

  Document each change as a numbered rule, so the list is reusable for other ports:

  1. **Remove DOS interrupt-vector installs** — `int 21h` with AH=25h has no
     CP/M-86 equivalent; drop it; ^C now triggers warm boot instead of a custom handler
  2. **Remap `int 21h` → `int 0E0h`, `AH=fn` → `CL=fn`** — FCB functions 01h–1Ah
     are numerically identical; `int 20h` (terminate) → BDOS fn 0 with DL=0
  3. **fn 27h and fn 28h are a trap** — in CP/M-86 these are disk utility calls
     (`DRV_ALLOCVEC`, `DRV_SETRO`), not file I/O; the correct replacements are
     fn 21h (`F_READRAND`) and fn 22h (`F_WRITERAND`); record unit = 128 bytes;
     pass record count not byte count; adjust random-record pointer arithmetic
     accordingly (÷128 before subtracting unused bytes)
  4. **Set DS=ES=CS at entry** — even `org 100h` programs receive DS=base-page
     segment, not CS, on entry; three `mov` instructions fix it
  5. **Set DMA segment** — BDOS fn 1Ah sets offset only; call fn 33h with DX=CS
     once at startup so file reads/writes land in the program's segment
  6. **PSP offset 06h is not available memory** — use `bin2cmd -m` to book the
     segment in the CMD header; use a compile-time equate for the memory ceiling
  7. **BDOS clobbers AX, BX, CX, DX, SI, DI, ES** (DOS only clobbers AX);
     protect `print_char` with push/pop of all those registers to cover every caller

  **Zero-page / PSP compatibility table** — which offsets are safe, which are not
  (reproduce the table from this plan)

  **FCB compatibility** — FCB layout identical between DOS and CP/M-86;
  FCB1 at 5Ch, FCB2 at 6Ch, all field offsets (+1 name, +9 ext, +14 recsiz,
  +16 newname, +33 random record) unchanged

  **Known limitations**
  - ^C during input triggers warm boot, not graceful return to the `*` prompt
  - All text must fit in the 64 KB segment; edlin's A/W commands manage this

- [ ] Add a `run` target to the Makefile:
      ```makefile
      run: edlin.cmd
      	emu2 $< $(FILE)
      ```
      Usage: `make run FILE=myfile.txt`

- [ ] Update the Overview in this plan file to mark the overall goal complete.

### Relevant Context
- `edlin.asm` lines 1–42: source header already contains a good command reference
- This plan file: complete technical record of every change and its rationale
