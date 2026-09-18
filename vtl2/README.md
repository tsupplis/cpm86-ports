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

The startup `OK` is suppressed when a script is loaded, so a script's own output
comes first. It still appears in interactive mode, and when the named file
cannot be opened.

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

A VTL-2 primer, enough to read and write the examples. The manual is `vtl2.pdf`.

**Statements.** A line starting with a number is stored as part of the program;
anything else runs immediately. One space separates the line number from the
statement, and nothing else in the line may contain spaces:

    10 A=B+C        stored as line 10
    ?=A             executed now

**Variables** are single characters, `A`–`Z` and most punctuation, each holding
0–65535. Characters and numbers are interchangeable, so `A=A+1` on a letter
gives the next letter.

**Operators** are `+ - * /` and the tests `=` (equal), `<` (less than) and `>`
(greater than *or equal*). Tests yield 1 or 0. Evaluation is strictly left to
right — `A*X*X+B*X+C` is not what you expect — so parenthesise.

**System variables** do the work of keywords:

| | |
| --- | --- |
| `#` | current line number; assigning to it is a GOTO |
| `!` | line after the last GOTO, i.e. a subroutine return |
| `?` | the terminal: `?=A` prints, `A=?` inputs |
| `$` | same, but a single character |
| `%` | remainder of the last division |
| `'` | a random number |
| `*` | memory size |
| `&` | next free byte of program text |
| `:` | array element, `:3=7` sets element 3 |

**Control flow** falls out of `#` plus the fact that assigning 0 to it does
nothing. That makes an IF:

    20 #=(X=25)*50      if X=25 then goto 50, else carry on

and a subroutine call is `#=100` with `#=!` to return. There is no `END`; a
program stops when it runs off the last line.

**Printing.** `?=A` prints a number with no newline. A quoted string prints with
a newline unless followed by `;`. `?=""` prints a newline on its own.

    50 ?="TOTAL IS ";
    60 ?=T
    70 ?=""

**At the prompt.** `0` lists the program, a line number alone deletes that line,
`@` cancels the line being typed, `^H`/DEL rub out a character, and `?=*-&`
reports free memory. `&=` the program start erases the program. In this CP/M
port `>=` exits to the operating system.

## Gotchas

Collected the hard way while making the above work.

**The CP/M tools report success even when they fail.** `asm80` and `asm86` exit
0 and print `END OF ASSEMBLY` while flagging errors only as one-letter markers
in the listing, e.g. `U` for an undefined symbol next to the offending line.
Always read `vtl2.prn` and `vtl2.lst`; a `U` there silently assembles to zeroes.

**`asm80` rejects underscores in labels.** `script_init` assembles to an
undefined reference, so labels here are `scrini`, `scrrfl` and so on.

**XLT86 is easily upset.** It translates one instruction at a time and tracks
flags across blocks, and it splits the result into `CSEG` and `DSEG`. So:

- anything that reads code as data breaks, because opcodes change and code is no
  longer adjacent to data — this is exactly the `prnt2` bug above;
- `END begin` is not accepted, only a bare `END`;
- a new subroutine with two entry points, reached by jumps from the middle of an
  existing routine, was mistranslated badly enough that emu2 hit
  `unimplemented opcode F1` — `F1` being the *8080* encoding of `POP PSW`.

Prefer small inline edits that reuse existing labels over new blocks, and always
test `vtl2.cmd`, not just `vtl2.com`.

**Do not test by piping into the console.** After every message VTL-2 polls
console status and consumes a character. With a pipe a character is always
"ready", so input is eaten and the session turns to garbage. Test with a script
file, which is what `test.sh` does.

**Terminals send DEL, not backspace.** The original only recognised `08H`, so
the Backspace key inserted `7FH` as text instead of deleting.


## Build

    make            both binaries
    make clean
    ./test.sh       regression tests, both binaries

Note that `asm80`, `asm86` and `xlt86` report errors inside the listing files
(`vtl2.prn`, `vtl2.lst`) while still exiting successfully, so check those rather
than the exit status.
