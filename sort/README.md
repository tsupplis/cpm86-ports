# sort

Sort text files using the BSD external merge sort.

```
sort [-bdfinrumcp] [-tX] [-o file] [+field[.char] [-end]] [file ...]
```

| Option | Meaning |
| --- | --- |
| `-b` | Ignore leading blanks in keys. |
| `-d` | Dictionary order: letters, digits, and blanks. |
| `-f` | Fold letters to upper case for comparison. |
| `-i` | Ignore nonprinting characters. |
| `-n` | Compare signed decimal numbers. |
| `-r` | Reverse order. |
| `-u` | Retain one line per equal key. |
| `-m` | Merge already sorted input files. |
| `-c` | Check order; with `-u`, also reject equal keys. Ignores `-o`. |
| `-tX` | Use X as the field separator; letters default to lower case. |
| `-o file` | Write output, including safely sorting/merging into an input file. |
| `-p` | Page final screen output. |

Historical key positions start at zero: `+1n` sorts numerically on the
second field; a following `-2` ends the key before the third field.
Modern `-k` keys are not supported. `-T` is not a temporary-directory option;
`-tX` and `-TX` both specify a separator, since option case is ignored.
Separators follow sed's command-tail case convention: `-tA` selects lower
case `a`, `-t%A` selects upper case `A`, and `-t\%` selects a literal `%`.
This convention applies on DOS too. Only the separator is converted; input
text is unchanged. A single `-` operand reads standard input.

```
sort -n -o numbers.txt input.txt
sort +1n -o result.txt input.txt
sort -m -o combined.txt first.txt second.txt
```

Lines are limited to 1022 characters excluding the newline; longer lines
produce an error. A missing final newline is supplied. Keys are limited to
nine explicit ranges. Without `-f`, comparisons are case sensitive.

## Building and checking

```
make          # CP/M-86 .cmd and PC-DOS .com, using Aztec C86 4.2
make test     # both binaries under emu2; requires Python 3
make clean
```

Option letters accept either case because the CP/M-86 CCP uppercases the
command tail. `-?` prints a usage summary. `-o file` writes to a file;
file output disables `-p` regardless of option order. `-p` pauses every
23 output lines; any key continues and Ctrl-C quits. Paging uses BDOS
console I/O and must be checked interactively; the automated tests do not
wait for console input.

Input and output are text files. With no input file, the console is read.
DOS reports status 0 on success and 1 on error; CP/M-86 has no useful
process exit status. Diagnostics go to the console on CP/M-86.

Based on the BSD / DiscoBSD source supplied with this repository.

## Porting notes

Unix `brk`/`sbrk` allocation is replaced by a fixed 24 KiB arena. Larger
inputs spill to CP/M-compatible `SRT00AA.TMP` through `SRT00ZZ.TMP` names
in the current directory (up to 676 runs, including merge passes). Existing
temporary files are refused rather than overwritten. If all 676 names are exhausted, created runs are cleaned up before exiting.
Four-way merges fit
the CP/M runtime's open-file limit. Normal completion and reported errors
remove created temporary files; an external interruption may leave them.

Unix signals, process IDs, temporary paths, inode checks, `perror`, and
`_exit` are removed. Merge output is staged before opening its destination
to protect input/output aliases. Paging applies only to final output.
Text input uses Aztec `agetc` to recognize CP/M EOF padding. The tests
exercise multiple merge passes and unique output on larger inputs.

## Text line endings

LF and CRLF input are accepted; output uses CRLF. A final line without a
newline receives one. Ctrl-Z permanently ends each input stream, including
when it follows an unterminated line, in normal sorting and merge mode.
CP/M output is padded to 128-byte records with Ctrl-Z marking text EOF;
an empty output file may contain no records. Bare CR-only files are not
supported: Aztec text input discards CR.
