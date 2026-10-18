# tr

Translate, delete, or squeeze characters.

```
tr [-cdsp] [-o file] string1 [string2 [input]]
```

- `-c`: complement string1 (all non-NUL bytes outside the set).
- `-d`: delete characters in string1.
- `-s`: squeeze repeated characters from the translated destination set.
- `-o file`: output file; `-p`: page screen output.

Sets accept ranges (`a-z`) and backslash octal escapes (`\141` is lower
case a). A short destination set repeats its last character. NUL bytes are
ignored, as in the original source. Modern character classes such as
`[:alpha:]` are not supported.

An optional third operand supplies input without shell redirection. For
example, on CP/M-86:

```
tr -o upper.txt A-Z %A-%Z input.txt
tr -d \170 Z input.txt
```

The second example deletes lower case x; Z is an unused destination set.
Character sets follow sed's command-tail case convention: letters become
lower case by default; `%` toggles case for the rest of the current word.
Digits and `_` keep the toggle; punctuation resets it. Thus `A-Z` means
`a-z`, and `%A-%Z` means `A-Z`. `\%` is a literal percent. Octal escapes
remain available for exact character values. This convention applies to
both CP/M-86 and DOS, just as in sed. `--` ends option processing for a set beginning with a dash.

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

Added file input, file output, paging, option validation, and case-insensitive
options. Text input uses Aztec's `agetc` through `getchar` so CP/M text EOF
padding is not translated. Set parsing no longer writes into empty literals.

## Text line endings

LF and CRLF input are accepted; output uses CRLF through Aztec text I/O.
Ctrl-Z ends text input. CP/M output is padded to 128-byte records with
Ctrl-Z marking the end of the text. Bare CR is discarded by the runtime,
so old Macintosh CR-only files are not treated as separate lines.
`tr` preserves an unterminated final line.
