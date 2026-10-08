# cpu

Processor tools for CP/M-86: running 8080 code — by translating it, or by
emulating the 8080 — and identifying the CPU the system runs on.

| File | What it does | Needs |
| --- | --- | --- |
| `80x86.cmd` | translates 8080 `.ASM` source to 8086 `.A86` source | — |
| `vcpm.cmd` | runs CP/M-80 `.CPM` binaries in the V20's 8080 mode | NEC V20 / V30 |
| `z80.cmd` | interprets 8080 binaries in software | — |
| `cpuid.cmd` | reports the processor type: 8088, 8086, 286 or 386+ | — |

The two emulators expect the 8-bit program renamed from `.COM` to `.CPM`, so it
doesn't collide with CP/M-86's own `.CMD`.

## 80X86 — 8080-to-8086 source translator

Harold V. McIntosh, Universidad Autónoma de Puebla, 1984. See `80x86.md`.

    80X86 [X:]FILE[.XXX]

Reads an 8080 `.ASM` file and writes `FILE.A86`. Every opcode and the `db`, `dw`,
`ds`, `equ`, `org`, `end` directives are translated. Input may be squeezed.

It is conservative rather than clever. The two classic translation traps are
handled by generating slightly longer sequences:

- `inx`/`dcx` don't touch flags but `inc`/`dec` do, so pointer arithmetic between
  a compare and a conditional jump is wrapped in `pushf`/`popf`
- the 8080 pushes A as the *high* byte of the PSW, the 8086 uses `al` as the
  *low* byte, so `push psw` / `pop psw` become `lahf`/`sahf` sequences

It also rewrites `call BDOS` / `call 0005H` / `call X0005` into `int bdos`, and
strips the label colons that ASM86 rejects on `db`, `dw`, `rb` and `rw`.

What it won't fix, and you'll have to by hand: labels colliding with ASM86
reserved words (`byte`, `wait`, `ss`, `dd`), `$` in identifiers, absolute
addresses such as the default FCB, and data alignment that mattered. Add
`bdos equ 0E0H` yourself. A program that manipulates 8080 opcodes *as data* —
an assembler, a debugger — is beyond it entirely.

### Which 80X86 this is

Four generations of this program exist. This is the last one: the 80X86 feature
set (BDOS rewriting, colon stripping) with the source hand-cleaned — the
machine-generated `jz Gnnn ! jmp target ! Gnnn:` trampolines replaced by direct
conditional jumps, the unnecessary `pushf`/`popf` guards dropped, and `@` removed
from generated label names. The `lahf`/`sahf` guards that are actually needed are
untouched.

Dropped from earlier revisions: `80T86.CNV` (the original, written in McIntosh's
CNVRT rewriting language), `80T86.ASM` (its 8080 hand-coding), `80T86.A86` (that
source run through itself), and the machine-translated `80X86.A86`.

## VCPM — V20 hardware emulator

Stephen Hunt, 1988. Version 1.5.

    VCPM PROGRAM.CPM [command tail]

Uses the `BRKEM`/`RETEM`/`CALLN` instructions of the NEC V20 (µPD70108) and V30
(µPD70116), which execute 8080 code natively. So it runs at full processor speed
rather than interpreting.

The most complete of the emulators here: a full 8080 BIOS jump table including
the disk routines, drive allocation table, DPH/DPB/DPT, both default FCBs parsed
from the command line, and a 62.7K TPA. The source carries a memory map and the
8086↔8080 register mapping.

Requires `GENCMD ... EXTRA[M1000]` to reserve 64K for the extra segment — the
Makefile does this. Without it the system will crash on the first 8080 program.

Disk BIOS calls need CP/M-86 BDOS 2.x. Under MP/M-86 or Concurrent CP/M-86 it
detects the version and terminates 8080 mode cleanly instead of guessing.

## Z80 — software emulator

Pat Hester, 1983; adapted for CP/M-86 by Bill Earnest, 1984. Version 1.2. See
`z80.md`.

    Z80 filename.typ

Interprets 8080 opcodes in software, so unlike VCPM it needs no special CPU — but
it is correspondingly slow. Mostly 8080, with a handful of Z80 instructions
(relative jumps, register exchanges). 48K TPA.

Programs must go through addresses 0 and 5 only; the BIOS jump table is off
limits except for CONIN, CONOUT, CONST and LIST. The original documentation's own
assessment: *"has more bugs than the Jersey swamps."*

**This does not currently build.** `cpm86_asm86` produces an empty `.h86`, so
`z80.cmd` has no code segment. The `PAGEWIDTH` and `TITLE` lines at the top are
RASM86 directives that DRI's ASM86 doesn't accept. Building it via the
`RASM86` + `LINK86` path the Makefile already defines would be the fix.

## CPUID — processor detection

Richard C. Leinecker; rewritten as a stand-alone program and translated to
CP/M-86 by Kirk Lawrence.

    CPUID

Prints which processor the machine contains, using the classic flag and
prefetch-queue tests:

- **8088 / 8086 vs 286+** — FLAGS bits 12–15 always read back as 1 on the
  8088/8086, and can be cleared on a 286 or later
- **8088 vs 8086** — self-modifying code 5 bytes ahead of IP takes effect only
  on the 8088, whose prefetch queue is 4 bytes instead of 6
- **286 vs 386+** — only a 386 or later keeps the NT/IOPL bits (`7000h`) set in
  FLAGS

A small 8080-model program (CS = DS = ES), built with a plain `GENCMD`.

The source also contains an NEC V20/V30 check: Intel CPUs lose the `REP` prefix
when a `REP LOCK LODSB` is interrupted, while NEC CPUs don't. **That check never
runs as written.** It is only called on the 286+ branch, but a V20/V30 takes the
8088/8086 branch because its FLAGS bits 12–15 behave like the 8086's. Its
result is then overwritten, and the `veetwen` message is never printed. So a
V20 is reported as an 8088 and a V30 as an 8086. Also, on a 386 or later,
`LOCK LODSB` raises an invalid-opcode exception.

## Building

    make            # 80x86.cmd, vcpm.cmd, z80.cmd, cpuid.cmd
    make test       # build a disk image and boot it under PCE

`make test` needs `cpmtools` for the image and `pce-ibmpc` for the emulator;
`cpm86.cfg` points PCE at an IBM XT with CP/M-86 on drive A and the test image on
B. Note that PCE emulates an 8088, not a V20, so **VCPM cannot be tested there** —
it needs real V20/V30 hardware or an emulator that implements 8080 mode.

## Samples

| File | Notes |
| --- | --- |
| `ver.com` / `ver.cpm` | reports the CP/M version — quickest check that an emulator works |
| `turbo.cpm`, `turbo.msg`, `turbo.ovr` | Turbo Pascal 3, a realistic workout |
| `test1b.asm` | small 8080 source for 80X86 |
| `xlt86.asm`, `xlt86d.asm` | DRI's own XLT86 translator, in 8080 source — the large-input case |

## Licensing

None of this is open source; it is 1980s hobbyist code preserved as found.

- **80X86** — Copyright © 1984 Universidad Autónoma de Puebla. No grant stated.
- **VCPM** — Copyright © 1988 Stephen Hunt. No grant, but no restrictions stated
  either.
- **Z80** — no notice at all.
- **CPUID** — marked freeware by its banner (Richard C. Leinecker, CP/M-86
  version by Kirk Lawrence).

Two other V20 emulators were considered and left out on licensing grounds:
Thomas M. Langley's **SWV20** (1986), which supports Concurrent DOS and CompuPro
`SW!` shell integration but is marked *"commercial and Government use
prohibited"*; and S. Kluger's **EMUL** (1985), which demands written
authorization and a $20-per-copy royalty, handles console I/O only, and is
written in TurboDOS TASM syntax that no assembler here accepts.
