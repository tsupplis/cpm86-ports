# diff

Compare two text files line by line.

## Usage

```
diff [-bwit] [-c[N]] [-e|-f|-n] [-D name] [-o file] [-p] file1 file2
```

| Option | Meaning |
| --- | --- |
| `-b` | Ignore trailing blanks and differences in runs of blanks. |
| `-w` | Ignore all blanks. |
| `-i` | Ignore case. |
| `-t` | Expand tabs in output. |
| `-c[N]` | Show N lines of context; default 3. |
| `-e`, `-f`, `-n` | Produce an ed script, reverse script, or numbered reverse script. |
| `-D name` | Merge files with conditional preprocessor directives. |

Normal output uses `a` (add), `c` (change), and `d` (delete); `<` marks lines
from the first file and `>` marks lines from the second.

## Examples

```
diff old.txt new.txt
diff -c -o changes.txt old.txt new.txt
diff -i -p old.txt new.txt
diff -D %FEATURE -o merged.c old.c new.c
```

## Notes

Option letters accept either case. `-o file` writes output and disables `-p`
in either order. `-p` pauses every 23 output lines; any key continues and
Ctrl-C quits. `-?` shows help.

Text input accepts LF/CRLF and stops at Ctrl-Z; output uses CRLF. Bare CR-only
files are not supported.

The `-D` name follows the `%` case convention: `NAME` becomes `name`,
`%NAME` stays `NAME`. Attached names such as `-D%NAME` also work.
Filenames are unchanged. Only two-file comparisons are supported.
A missing final newline alone is not a difference.

DOS exit status: 0 equal, 1 different, 2 error. CP/M-86 has no useful exit status.

## Build

`make` builds CP/M-86 `.cmd` and PC-DOS `.com` binaries with Aztec C86 4.2.
`make test` runs emulator checks; `make clean` removes build output.

BSD Unix / RetroBSD / DiscoBSD source; BSD 3-Clause.
