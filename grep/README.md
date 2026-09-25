grep
====

`grep` — search a file for a pattern.

This is the RetroBSD / DiscoBSD descendant of the Berkeley `grep` ported to
CP/M-86 (and PC-DOS) using the Aztec C86 4.2 cross compiler. Only `grep` is
provided; there is no `egrep` or `fgrep` in this port.

## Synopsis

```
grep [-bcehilnsvwy?] expression [file ...]
```

With no file argument the console is read.

## Options

| Option | Meaning |
| --- | --- |
| `-b` | Each line is preceded by the block number on which it was found. |
| `-c` | Only a count of matching lines is printed. |
| `-e` | The next argument is the expression, even if it begins with `-`. |
| `-h` | Never prefix output lines with the file name. |
| `-i` | Ignore the case of letters when comparing (same as `-y`). |
| `-l` | List the names of the files that contain a match, once each. |
| `-n` | Each line is preceded by its line number in the file. |
| `-s` | Silent: print nothing, only set the exit status. |
| `-v` | Print all lines *except* those matching. |
| `-w` | Search for the expression as a whole word. |
| `-y` | Same as `-i`. |
| `-?` | Print a usage summary. |

Option letters may be given in either case, because the CP/M-86 CCP folds the
command tail: `-N` and `-n` both mean `-n`.

The file name is shown in front of each line when there is more than one input
file, unless `-h` is given.

## Expressions

Patterns are limited regular expressions in the style of `ex`:

| Construct | Matches |
| --- | --- |
| *c* | The character *c*, if it has no special meaning. |
| `.` | Any single character. |
| `^` | The beginning of the line (only as the first character). |
| `$` | The end of the line (only as the last character). |
| `[...]` | Any one character from the set; ranges may be written `a-z0-9`. |
| `[^...]` | Any one character *not* in the set. |
| *re*`*` | Zero or more matches of the preceding one-character expression. |
| `\<` `\>` | The beginning / end of a word. |
| `\(`...`\)` | A tagged sub-expression, up to nine of them. |
| `\1`..`\9` | The text matched by the *n*-th tagged sub-expression. |
| `\`*c* | The character *c* literally. |

## Case on CP/M-86

The CP/M-86 CCP folds the command tail to upper case before a program sees it,
so a lower case pattern simply cannot be typed. This port therefore reads the
expression through a small case layer:

- letters match **lower case** by default, whatever case they were typed in;
- `%` **toggles** the case of the rest of the current word;
- a word is made of letters, digits and `_` — any other character resets the
  toggle back to lower case;
- `\%` is a literal `%`.

Examples, written the way the CCP delivers them:

| Typed | Pattern actually searched for |
| --- | --- |
| `HELLO` | `hello` |
| `%HELLO` | `HELLO` |
| `%H%ELLO` | `Hello` |
| `%AAA-BBB` | `AAA-bbb` |
| `%AAA_BBB1C` | `AAA_BBB1C` |
| `%AAA_BBB1%C` | `AAA_BBB1c` |
| `%AA;BB` | `AA;bb` |
| `100\%` | `100%` |
| `[%A-%Z]` | `[A-Z]` |

Because `-` and `;` are not word characters they reset the toggle, while `_`
and the digit `1` keep it, which is what makes `%AAA_BBB1C` come out entirely
in upper case.

Note that regular expression metacharacters (`.`, `*`, `[`, `^`, `$`, `\`) are
not word characters either, so each of them also resets the toggle.

## Diagnostics

Exit status is 0 if any matches were found, 1 if none, and 2 for a bad
expression or an inaccessible file.

CP/M-86 terminates programs through BDOS function 0, which carries no exit
status, so only `grep.com` reports these values usefully. The Aztec `c86`
library also sends `stderr` to the console, so under CP/M-86 error messages
appear on standard output.

## Building

```sh
make          # grep.cmd (CP/M-86) and grep.com (PC-DOS)
make test     # run the regression suite under emu2
make clean
```

See https://github.com/tsupplis/cpm86-crossdev for the cross-development
environment.

## Porting notes

- `perror` was replaced by a plain message: the Aztec CP/M-86 library has no
  `sys_errlist`/`sys_nerr`.
- `printf("%D", ...)`, a V7 spelling for a long, became `%ld`.
- A failed `freopen` used to fall through and read the console instead of the
  missing file, which hung the program. It now returns immediately.
- Option letters are folded so that the upper case command tail works.
- The `%` case layer described above was added; it has no counterpart in the
  original.

## Bugs

Lines are limited to 1024 characters; longer lines are truncated.

`-b` always reports block 0: the upstream source never updates the counter.

## See also

`ex`(1), `sed`(1)

## License

BSD 3-Clause — see [LICENSE.md](LICENSE.md). Copyright (c) 2020-2026 DiscoBSD,
(c) 2014 RetroBSD, (c) 1980 Regents of the University of California.
