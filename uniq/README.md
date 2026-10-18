# uniq

Filter adjacent duplicate lines. Sort first to collect nonadjacent duplicates.

## Usage

```
uniq [-cdu] [-o file] [-p] [-fields] [+chars] [input [output]]
```

With no input file, read the console. A positional output filename is also
accepted instead of `-o file`.

| Option | Meaning |
| --- | --- |
| `-c` | Prefix each group with its count. |
| `-d` | Print repeated groups only. |
| `-u` | Print groups occurring only once. |
| `-fields` | Skip this many whitespace-separated fields when comparing. |
| `+chars` | Skip this many additional characters. |

The last of `-c`, `-d`, and `-u` takes effect.

## Examples

```
uniq -c -o counts.txt sorted.txt
uniq -d -p sorted.txt
uniq -1 input.txt output.txt
```

## Notes

Option letters accept either case. `-o file` writes output and disables `-p`
in either order. `-p` pauses every 23 output lines; any key continues and
Ctrl-C quits. `-?` shows help.

Text input accepts LF/CRLF and stops at Ctrl-Z; output uses CRLF. Bare CR-only
files are not supported.

Comparisons are case sensitive. Lines may contain up to 999 characters;
longer lines produce an error. A final unterminated line receives CRLF.

## Build

`make` builds CP/M-86 `.cmd` and PC-DOS `.com` binaries with Aztec C86 4.2.
`make test` runs emulator checks; `make clean` removes build output.

BSD Unix / DiscoBSD source; BSD 3-Clause.
