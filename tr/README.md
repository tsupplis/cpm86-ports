# tr

Translate, delete, or squeeze repeated characters.

## Usage

```
tr [-cds] [-o file] [-p] string1 [string2 [input]]
```

With no input file, read the console. To supply a file when deleting, give
an unused second set, as in the second example below.

| Option | Meaning |
| --- | --- |
| `-c` | Complement the first character set. |
| `-d` | Delete characters in the first set. |
| `-s` | Squeeze repeated characters in the destination set. |

Sets accept ranges (`a-z`) and octal escapes (`\141` for lower case a).
A short destination set repeats its last character. `--` ends options for
a set beginning with `-`.

## Examples

```
tr -o upper.txt A-Z %A-%Z input.txt
tr -d X Z input.txt
tr -s X X input.txt
```

The second example deletes lower case x; the third squeezes repeated x.

## Notes

Option letters accept either case. `-o file` writes output and disables `-p`
in either order. `-p` pauses every 23 output lines; any key continues and
Ctrl-C quits. `-?` shows help.

Text input accepts LF/CRLF and stops at Ctrl-Z; output uses CRLF. Bare CR-only
files are not supported.

Letters in command-line text default to lower case. `%` toggles case within
a word; punctuation resets it, while digits and `_` keep it. `\%` is a
literal percent. This convention also applies on DOS.

Both sets use this convention: `A-Z` means `a-z`; `%A-%Z` means `A-Z`.
NUL bytes are ignored. Character classes such as `[:alpha:]` are unsupported.
A final unterminated line stays unterminated.

## Build

`make` builds CP/M-86 `.cmd` and PC-DOS `.com` binaries with Aztec C86 4.2.
`make test` runs emulator checks; `make clean` removes build output.

BSD Unix / DiscoBSD source; BSD 3-Clause.
