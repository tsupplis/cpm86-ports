sed
===

`sed` — stream editor.

This is the BSD `sed` ported to CP/M-86 (and PC-DOS) using the Aztec C86 4.2
cross compiler.  The source descends from the same BSD Unix-tools tree as the
`grep` port in this collection.

## Synopsis

```
sed [-ngop?] [-e script] [-f file] [-o outfile] [script] [file ...]
```

With no file argument, the standard input (console) is read.

If no `-e` or `-f` option is given, the first non-option argument is taken as
the script and the remaining arguments are the input files.

## Options

| Option | Meaning |
| --- | --- |
| `-e script` | Add *script* to the list of commands to execute. |
| `-f file` | Read the script from *file* instead of the command line. |
| `-g` | Apply all substitutions globally by default (same as the `g` flag on every `s` command). |
| `-n` | Suppress the default print of each line; only `p` and `P` commands produce output. |
| `-o outfile` | Write all output to *outfile* instead of the screen; CP/M-86 has no pipes. Incompatible with `-p` — if both are given, `-o` wins and `-p` is ignored. |
| `-p` | Pause after 23 lines of output until a key is pressed (`^C` quits); CP/M-86 has no pipes to a pager. Ignored if `-o` is also given. |
| `-?` | Print a usage summary. |

Option letters may be given in either case, because the CP/M-86 CCP folds the
command tail: `-N` and `-n` both mean `-n`.

## Commands

| Command | Meaning |
| --- | --- |
| `[addr] a\` *text* | Append *text* after each addressed line. |
| `[addr1[,addr2]] b` [*label*] | Branch to *label*; if no label, branch to end of script. |
| `[addr1[,addr2]] c\` *text* | Delete addressed lines and replace with *text*. |
| `[addr1[,addr2]] d` | Delete the pattern space; start next cycle. |
| `[addr1[,addr2]] D` | Delete up to and including the first newline in the pattern space; restart. |
| `[addr1[,addr2]] g` | Replace the pattern space with the hold space. |
| `[addr1[,addr2]] G` | Append a newline and the hold space to the pattern space. |
| `[addr1[,addr2]] h` | Copy the pattern space to the hold space. |
| `[addr1[,addr2]] H` | Append a newline and the pattern space to the hold space. |
| `[addr] i\` *text* | Insert *text* before each addressed line. |
| `[addr1[,addr2]] l` | Print the pattern space showing non-printable characters visibly. |
| `[addr1[,addr2]] n` | Print the pattern space (unless `-n`); read the next line. |
| `[addr1[,addr2]] N` | Append the next line to the pattern space. |
| `[addr1[,addr2]] p` | Print the pattern space. |
| `[addr1[,addr2]] P` | Print up to the first newline in the pattern space. |
| `[addr] q` | Print the pattern space (unless `-n`) then quit. |
| `[addr] r` *file* | Append the contents of *file* after the addressed line. |
| `[addr1[,addr2]] s/`*re*`/`*rhs*`/`[`gp`] | Substitute; `g` replaces all occurrences, `p` prints if a substitution was made. |
| `[addr1[,addr2]] t` [*label*] | Branch to *label* if any `s` substitution has been made since the last line was read or `t` was tested. |
| `[addr1[,addr2]] w` *file* | Write the pattern space to *file*. |
| `[addr1[,addr2]] x` | Exchange pattern and hold spaces. |
| `[addr1[,addr2]] y/`*str1*`/`*str2*`/` | Transliterate characters in *str1* to corresponding characters in *str2*. |
| `[addr1[,addr2]] =` | Print the current line number. |
| `[addr1[,addr2]] {` | Begin a group of commands (closed by `}`). |
| `[addr1[,addr2]] !` *cmd* | Apply *cmd* to lines that do **not** match the address. |

Addresses may be a line number, `$` (last line), or a `/`*regex*`/` pattern.

## Case on CP/M-86

The CP/M-86 CCP folds the entire command tail to upper case before a program
sees it, so lower-case letters in a `-e` script or an inline script simply
cannot be typed directly.  This port therefore reads `-e` arguments and the
inline script through the same `%`-toggle case layer used by `grep`:

- letters match **lower case** by default, whatever case they were typed in;
- `%` **toggles** the case of the rest of the current word;
- a word is made of letters, digits and `_` — any other character (including
  regex metacharacters) resets the toggle back to lower case;
- `\%` is a literal `%`.

Examples, written as the CCP delivers them (all upper case):

| Typed | Script actually compiled |
| --- | --- |
| `S/FOO/BAR/` | `s/foo/bar/` |
| `S/%FOO/%BAR/` | `s/FOO/BAR/` |
| `S/%F%OO/%B%AR/` | `s/FoO/BaR/` |
| `/HELLO/D` | `/hello/d` |
| `/%HELLO/D` | `/HELLO/d` |
| `S/100\%/DONE/` | `s/100%/done/` |

The `-f` option reads the script from a file.  File content is **not** touched
by the CCP, so no case folding is applied — you can write the script in the
normal mixed-case POSIX style and save it to disk.

## Building

```sh
make          # sed.cmd (CP/M-86) and sed.com (PC-DOS)
make clean
```

See https://github.com/tsupplis/cpm86-crossdev for the cross-development
environment.

## Porting notes

- `#include <fcntl.h>` and `#include <unistd.h>` removed; `sed1.c` used
  low-level `open()`/`read()`/`close()` POSIX calls which Aztec C86 does not
  provide.  Replaced throughout with `fopen()`/`fread()`/`fclose()`.
- All global variables were definitions in `sed.h`, causing "multiply defined"
  linker errors when the header was included in both translation units.  Moved
  all definitions to `sed0.c`; `sed.h` now contains only `extern` declarations.
- Duplicate declarations of `cp`, `reend`, `lbend`, and `depth` in `sed.h`
  removed.
- Buffer sizes reduced: `LBSIZE` 4000→1024, `RESIZE` 10000→4096, to fit
  CP/M-86's 64 KB TPA.
- Option letters are folded to lower case so the upper-case command tail works.
- The `%` case layer (see above) was added to `-e` arguments and the inline
  script.  It has no counterpart in the original.
- The `-o outfile` option was added because CP/M-86 has no pipes.  It
  redirects stdout to a file via `freopen`; all existing output code is
  transparent to this change.
- The `-p` pause option was added for the same no-pipe reason.  It uses BDOS
  direct console I/O (function 6) for the prompt and key read, identical to
  the `grep` implementation.  `-o` and `-p` are mutually exclusive; `-o` wins.
- The `w` script command writes to a named file.  If the script is supplied via
  `-e`, the filename in the `w` argument will have been case-folded; use `-f`
  to avoid this.
- `perror` is not used; error messages are plain `fprintf(stderr, ...)` — the
  Aztec CP/M-86 library has no `sys_errlist`/`sys_nerr`.

## Bugs

Lines are limited to 1024 characters; longer lines are silently truncated.

The regex buffer (`respace`) is limited to 4096 bytes; complex scripts with
many addresses and substitutions may hit this limit.

In-place editing (`-i`) is not supported.  Use `-o outfile` to write output to
a file, then rename manually.

The `w` command filename is case-folded when it appears in a `-e` script
(CP/M-86 CCP uppercases the tail); use `-f` to supply scripts with `w`
commands.

## See also

`grep`(1)

## License

BSD 3-Clause — see [LICENSE.md](LICENSE.md).  Derived from the BSD Unix `sed`
as carried in the RetroBSD / DiscoBSD source tree.
