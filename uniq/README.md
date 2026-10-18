# uniq

Filter adjacent duplicate lines. Sort the input first to collect duplicates
that are not already adjacent.

```
uniq [-cdu] [-p] [-o file] [-fields] [+chars] [input [output]]
```

- `-c`: prefix each group with its count.
- `-d`: print repeated groups only.
- `-u`: print groups occurring only once.
- `-fields`: ignore that many initial whitespace-separated fields.
- `+chars`: ignore that many additional characters.

The last of `-c`, `-d`, and `-u` takes effect. Comparisons remain case
sensitive. The historical positional output operand is also supported.
Lines may contain up to 999 characters; longer lines produce an error.
A final line without a newline is processed and receives a newline.

```
uniq -c -o counts.txt sorted.txt
uniq -p sorted.txt
```

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

Added output and paging options, case-insensitive options, and usage help.
Bounded line input prevents buffer overflow and retains a final unterminated
line. Existing field and character skipping behavior is preserved.

## Text line endings

LF and CRLF input are accepted; output uses CRLF through Aztec text I/O.
Ctrl-Z ends text input. CP/M output is padded to 128-byte records with
Ctrl-Z marking the end of the text. Bare CR is discarded by the runtime,
so old Macintosh CR-only files are not treated as separate lines.
`uniq` retains an unterminated final line and adds CRLF when printing it.
