# sort

Sort text lines, merge sorted files, or check their order.

## Usage

```
sort [-bdfinrumc] [-tX] [-o file] [-p] [+field[.char] [-end]] [file ...]
```

With no input file, or a `-` operand, read the console.

| Option | Meaning |
| --- | --- |
| `-b` | Ignore leading blanks in keys. |
| `-d` | Compare letters, digits, and blanks only. |
| `-f` | Ignore letter case. |
| `-i` | Ignore nonprinting characters. |
| `-n` | Compare signed decimal numbers. |
| `-r` | Reverse order. |
| `-u` | Keep one line per equal key. |
| `-m` | Merge already sorted inputs. |
| `-c` | Check order; with `-u`, also reject duplicates. Ignores `-o`. |
| `-tX` | Use X as the field separator. |

Keys use zero-based positions: `+1n` compares numerically from the second
field; a following `-2` ends the key before the third. Modern `-k` is unsupported.

## Examples

```
sort -n -o numbers.txt input.txt
sort +1n -o result.txt input.txt
sort -m -o combined.txt first.txt second.txt
```

## Notes

Option letters accept either case. `-o file` writes output and disables `-p`
in either order. `-p` pauses every 23 output lines; any key continues and
Ctrl-C quits. `-?` shows help.

Text input accepts LF/CRLF and stops at Ctrl-Z; output uses CRLF. Bare CR-only
files are not supported.

Comparisons are case sensitive unless `-f` is used. Separators follow the
`%` case convention: `-tA` selects `a`, `-t%A` selects `A`, and `-t\%`
selects `%`, on both CP/M-86 and DOS. `-T` also means separator, not temp directory.

Lines may contain up to 1022 characters, with at most nine explicit keys.
A final unterminated line receives CRLF. `-o` may name an input file.
Large inputs use `SRT00??.TMP` files in the current directory; leave room
for them. Existing temporary files are never overwritten.

## Build

`make` builds CP/M-86 `.cmd` and PC-DOS `.com` binaries with Aztec C86 4.2.
`make test` runs emulator checks; `make clean` removes build output.

BSD Unix / DiscoBSD source; BSD 3-Clause.
