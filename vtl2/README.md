# VTL-2

MITS VTL-2 (Very Tiny Language) for the Altair 8800, by Frank McCoy, 1976.
CP/M port by Peter Schorn, 2006. See `vtl2.pdf` for the language manual.

Builds two binaries from the same `vtl2.asm`:

- `vtl2.com` — 8080 / CP/M-80, via `asm80` + `hexcom`
- `vtl2.cmd` — 8086 / CP/M-86, via `xlt86` + `asm86` + `gencmd`

## Changes from the original

**Builds for CP/M-86.** `END begin` became a bare `END` so XLT86 can translate
the source.

**Runs a script file.** An optional filename on the command line is read from
the default FCB and fed to the interpreter instead of the keyboard. At end of
file, input falls back to the console, so a script may either run a program or
just load one:

    vtl2 ex0.vtl        run a program
    vtl2 ex4.vtl        load a program, then type #=1 to run it
    vtl2                interactive, as before

**Rubout.** Both `^H` and DEL erase the previous character. On an empty line
they are ignored.

**8086 fix.** `prnt2` ended its digit loop by reading the byte after the
powers-of-ten table and testing for `7EH` — the opcode of the `MOV A,M` that
happened to follow it. XLT86 emits a different opcode and puts code and data in
separate segments, so the marker is now an explicit `DB 7EH`.

## Examples

| File | Notes |
| --- | --- |
| `ex0.vtl` | average of three constants |
| `ex1.vtl` | average of three inputs, read from the script |
| `ex2.vtl` | Fibonacci below 100 |
| `ex3.vtl` | Fibonacci, number of terms read from the script |
| `ex4.vtl` | program only, no `#=1` — loads and returns to the prompt |

## Language notes

Easy to get wrong when writing scripts:

- Expressions take no spaces. `A=B+C`, not `A = B + C`. The only space is the
  one after the line number.
- `#=1` runs a program; there is no `RUN`.
- `#=0` is ignored rather than stopping a program; run off the end instead.
- `?=A` prints a number without a newline. Use `?=""` for a newline.
- `>` means greater than *or equal to*.
- `0` lists the program, `>=` returns to CP/M.

## Build

    make            both binaries
    make clean

Note that `asm80`, `asm86` and `xlt86` report errors inside the listing files
(`vtl2.prn`, `vtl2.lst`) while still exiting successfully, so check those rather
than the exit status.
