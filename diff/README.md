diff
====

`diff` — compare two files line by line.

This is the BSD `diff` ported to CP/M-86 (and PC-DOS) using the Aztec C86 4.2
cross compiler.  The source descends from the same BSD Unix-tools tree as the
`grep` and `sed` ports in this collection.

Only file-to-file comparison is supported.  Directory comparison, stdin (`-`)
input, and the halfhearted (`-h`) fallback algorithm have been removed as they
depend on Unix pipes, `fork`, `exec`, or `/tmp` — none of which exist on
CP/M-86.

## Synopsis

```
diff [-bwite] [-c[N]] [-efn] [-D name] [-o outfile] [-p] [-?] file1 file2
```

## Options

| Option | Meaning |
| --- | --- |
| `-b` | Ignore trailing blanks; compress runs of blanks elsewhere. |
| `-w` | Ignore all blanks. |
| `-i` | Ignore case when comparing lines. |
| `-t` | Expand tabs on output (align columns). |
| `-e` | Produce an `ed` script that transforms *file1* into *file2*. |
| `-f` | Produce a reverse `ed` script (not directly usable by `ed`). |
| `-n` | Produce a numbered reverse `ed` script (used by `diff3`). |
| `-c[N]` | Produce a context diff with *N* lines of context (default 3). |
| `-D name` | Produce merged output with `#ifdef`/`#endif NAME` markers. |
| `-o outfile` | Write all output to *outfile* instead of the screen; CP/M-86 has no pipes. Incompatible with `-p` — if both are given, `-o` wins and `-p` is ignored. |
| `-p` | Pause after 23 lines of output until a key is pressed (`^C` quits); CP/M-86 has no pipes to a pager. Ignored if `-o` is also given. |
| `-?` | Print a usage summary. |

Option letters may be given in either case, because the CP/M-86 CCP folds the
command tail: `-B` and `-b` both mean `-b`.

## Output formats

### Normal (default)

Lines of the form:

```
line[,line] a|c|d line[,line]
< lines from file1
---
> lines from file2
```

Exit status is 0 if the files are identical, 1 if they differ, 2 on error.

### Context (`-c[N]`)

Displays *N* lines of surrounding context around each change.  The header
shows the two filenames; timestamps are omitted (CP/M-86 has no `ctime`).

### Editor scripts (`-e`, `-f`, `-n`)

Produce scripts suitable for feeding to `ed` (or compatible line editors) to
transform *file1* into *file2*.

### `#ifdef` merge (`-D name`)

Produces a merged file where changed sections are guarded with
`#ifdef NAME` / `#endif NAME` preprocessor directives.

## Case on CP/M-86

The CP/M-86 CCP folds the command tail to upper case before a program sees it.
Option letters are folded back to lower case internally, so `-B`, `-W`, `-I`
etc. all work.

File names on the command line are passed through the same `%`-toggle
case layer used by `grep` and `sed`: letters default to lower case, `%`
toggles to upper, `\%` is a literal `%`.  On CP/M-86 filenames are typically
upper case already (the CCP delivers them that way), so this is rarely needed.

## Building

```sh
make          # diff.cmd (CP/M-86) and diff.com (PC-DOS)
make test     # run the regression suite under emu2
make clean
```

See https://github.com/tsupplis/cpm86-crossdev for the cross-development
environment.

## Porting notes

- All POSIX/system headers (`sys/param.h`, `sys/stat.h`, `sys/dir.h`,
  `sys/wait.h`, `signal.h`, `fcntl.h`, `unistd.h`) removed.  A minimal
  `struct stat` stub with only `st_mode` is defined in `diff.h`; `stat()` is
  mapped to `diff_stat()` which probes with `fopen` to verify the file exists.
- `diffdir.c` removed entirely — directory comparison requires `opendir`,
  `readdir`, `fork`, `exec`, `pipe` and `wait`, none of which are available.
- `diffh.c` removed — the halfhearted fallback required a separate binary
  launched via `execv`; no process spawning on CP/M-86.
- `copytemp()` removed — reading from stdin into a temp file used
  `signal`, `mktemp("/tmp/…")`, `creat`, `read(0,…)`, `write`, `close` and
  `unlink`.  All gone; two explicit file arguments are now required.
- `splice()` removed — path manipulation for directory diff mode; not needed.
- `#include <a.out.h>` and the `N_BADMAG` binary-format check removed from
  `asciifile()`; replaced by a plain high-byte scan.
- `perror()` replaced throughout by `fprintf(stderr, …)`.
- The context diff header no longer shows timestamps (required `ctime` and
  `st_mtime`, neither available in the stat stub).
- `signal(SIGHUP/SIGINT/SIGPIPE/SIGTERM, done)` removed from `copytemp()`.
- Option letters folded to lower case before the switch (CP/M-86 CCP delivers
  upper case).
- `-o outfile` and `-p` (pause) added; same pattern as `grep` and `sed`.
- `-r`, `-l`, `-s`, `-S`, `-h` options removed (all depended on diffdir,
  diffh, or pr).

## Bugs

Files are limited by available heap (CP/M-86 TPA is 64 KB).  The algorithm
allocates approximately 6 × *n* words for files of *n* lines.  Very large
files will trigger the "files too big" error.

Binary files are detected by scanning for bytes with the high bit set; the
message "Binary files X and Y differ" is printed and `diff` exits with status 1.

The context diff header shows only the filenames, not timestamps.

In-place patching (`patch`) is not available on CP/M-86.

## See also

`grep`(1), `sed`(1)

## License

BSD 3-Clause — see [LICENSE.md](LICENSE.md).  Derived from the BSD Unix `diff`
as carried in the RetroBSD / DiscoBSD source tree.
