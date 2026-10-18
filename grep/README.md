# grep

Print lines that match a regular expression.

## Usage

```
grep [-bchilnpsvwy] [-o file] [-e] expression [file ...]
```

With no input file, read the console.

| Option | Meaning |
| --- | --- |
| `-b` | Show the block number (this port always reports 0). |
| `-c` | Print a matching-line count per file. |
| `-e` | Take the next argument as the expression, even if it starts with `-`. |
| `-h` | Suppress filename prefixes. |
| `-i`, `-y` | Ignore case. |
| `-l` | List filenames containing a match. |
| `-n` | Show line numbers. |
| `-s` | Suppress output; set exit status only. |
| `-v` | Select nonmatching lines. |
| `-w` | Match whole words. |

Expressions support `^`, `$`, `.`, `*`, `[set]`, `[^set]`, `\<`, `\>`,
`\(...\)`, and backreferences `\1` through `\9`.

## Examples

```
grep -n ERROR log.txt
grep -i -o matches.txt ERROR log.txt
grep -p ^%ERROR log.txt
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

For example, `ERROR` searches for `error`; `%ERROR` searches for `ERROR`.
Lines are limited to about 1024 characters. A final unterminated line is searched.

DOS exit status: 0 match, 1 no match, 2 error. CP/M-86 has no useful exit status.

## Build

`make` builds CP/M-86 `.cmd` and PC-DOS `.com` binaries with Aztec C86 4.2.
`make test` runs emulator checks; `make clean` removes build output.

BSD Unix / RetroBSD / DiscoBSD source; [BSD 3-Clause](LICENSE.md).
