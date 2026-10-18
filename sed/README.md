# sed

Edit text using a script, without changing the input files.

## Usage

```
sed [-ng] [-o file] [-p] [-e script] [-f scriptfile] [script] [file ...]
```

Use `-e` to add a command-line script or `-f` to read one from a file; both
may be repeated. Otherwise, the first operand is the script. With no input
file, read the console.

| Option | Meaning |
| --- | --- |
| `-n` | Suppress automatic printing. |
| `-g` | Replace all matches by default in every substitution. |
| `-e script` | Add a script from the command line. |
| `-f file` | Add a script from a file, preserving its case. |

| Script command | Meaning |
| --- | --- |
| `s/old/new/g` | Replace every match on a line; omit `g` for the first. |
| `p`, `d`, `q` | Print, delete, or quit. |
| `n`, `N` | Read the next line, or append it to the pattern space. |
| `h`, `g`, `x` | Save, restore, or exchange the hold space. |
| `H`, `G`, `D`, `P` | Append to hold/pattern space, delete/print its first line. |
| `y/abc/xyz/` | Translate characters. |
| `r file`, `w file` | Read text after the current line, or write the current line. |
| `=` | Print the line number. |

Prefix commands with a line number, `$` (last line), `/pattern/`, or an
address range such as `2,5` or `/start/,/end/`.

## Examples

```
sed -o result.txt S/OLD/NEW/G input.txt
sed -n -e /ERROR/P input.txt
sed -f edits.sed -p input.txt
```

## Notes

Option letters accept either case. `-o file` writes output and disables `-p`
in either order. `-p` pauses every 23 output lines; any key continues and
Ctrl-C quits. `-?` shows help.

Text input accepts LF/CRLF and stops at Ctrl-Z; output uses CRLF. Bare CR-only
files are not supported.

Letters in command-line text default to lower case. `%` toggles case within
a word; punctuation resets it, while digits and `_` keep it. `\%` is a
literal percent. This convention also applies on DOS.

This affects inline and `-e` scripts: `S/OLD/NEW/G` becomes `s/old/new/g`.
Use `%N` for the uppercase `N` command, or use `-f` for scripts with mixed
case or filenames. Script files are never case-folded.

Lines are limited to 1024 characters. A final unterminated line is processed
and normally receives CRLF; an `r` command preserves its file's final newline.

## Build

`make` builds CP/M-86 `.cmd` and PC-DOS `.com` binaries with Aztec C86 4.2.
`make test` runs emulator checks; `make clean` removes build output.

BSD Unix / RetroBSD / DiscoBSD source; BSD 3-Clause.
