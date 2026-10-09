# Plan: Port `sed` to CP/M-86

## Overview

Port the existing BSD `sed` (two-file source: `sed0.c` + `sed1.c`) to CP/M-86 using
the same Aztec C86 4.2 cross-build approach already established by `grep` and `dc`.

The work involves:
1. Updating `prospects.md` — mark `awk` as near-rejected (lex/yacc dependency), elevate `sed` to in-progress.
2. Replacing the unworkable POSIX Makefile with an Aztec-style one (grep template).
3. Porting the source: remove/stub POSIX-only headers, fold option letters to upper-case
   (CP/M-86 CCP always uppercases the command tail), apply the same `%` case-layer from
   `grep` to `-e` script arguments, add a `-o <file>` output-redirect option (replaces
   piping), drop or guard any pipe/shell-escape code.
4. Adding `-o <file>` also to `grep` (same no-pipe rationale, noting it is incompatible
   with `-p`).
5. Writing a `sed/README.md` of the same quality as `grep/README.md`.
6. Updating the root `README.md` (Included projects table + root Makefile SUBDIRS).

### Non-goals

- No `awk` port (lex/yacc dependency makes it a near-showstopper on CP/M-86).
- No `diff` port (out of scope for this plan).
- No feature additions beyond what is needed for a clean CP/M-86 build and usability.

### Key design decisions (recorded here for implementer)

#### `-p` (grep pause) vs `-o` (output file) incompatibility
`-p` in grep uses BDOS direct console I/O (`bdos(6, 0xFF)`), which is meaningless when
stdout has been redirected to a file. The two options are mutually exclusive. If both
are supplied, `-o` wins (stdout is redirected, `-p` is silently ignored). Document this
in `grep/README.md`.

Sed has **no `-p` command-line flag** — `p` in sed is a *script command* (`PCOM`), not
a CLI option — so there is no conflict in sed.

#### Case layer placement in sed
The CCP uppercases the *entire command tail* before passing it to the program. This
affects:
- `-e SCRIPT` arguments — the script text arrives uppercased; the `%`-toggle
  `casefold()` function (from grep) must be applied to each `-e` argument before
  `fcomp()` compiles it, so the user can write mixed-case literals.
- The first non-option argument (inline script) — same treatment.
- `-f FILE` arguments — the *file content* is read from disk and is **not** touched by
  the CCP; **no casefold** is applied.

#### sed CLI options — keep / add / remove

| Option | Action | Reason |
|--------|--------|--------|
| `-n`   | Keep   | Standard suppress-print; no pipe dependency |
| `-e`   | Keep + casefold | Script on command tail — CCP uppercases it |
| `-f`   | Keep, no casefold | Script from file — CCP does not touch file contents |
| `-g`   | Keep   | Global substitution flag; pure in-memory |
| `-o`   | **Add** | CP/M-86 has no pipes; redirect stdout to file via `freopen` |
| `?`    | **Add** | Usage summary (same as grep's `-?`) |

No options need to be removed from the current four (`-n -e -f -g`). The `w` script
command writes to a named file embedded in the script; if the script comes via `-e` the
filename will be uppercased by the CCP/casefold layer — noted as a known limitation in
the README bugs section.

---

## Sub-Task 1 — Update `prospects.md`

**Intent**  
Reflect the new strategic decision: `awk` is de-prioritised (almost a showstopper), `sed`
is moving to active work.

**Expected Outcomes**  
- `awk` row carries a clear ❌ or heavy ⚠️ note: "depends on lex/yacc — near showstopper
  on CP/M-86; focus redirected to sed".
- `sed` row updated to reflect active work / in-progress status.

**Todo List**  
- [ ] Edit `prospects.md` awk row: change status to ❌ Rejected (or a strong ⚠️), add
  note "lex/yacc dependency is a near showstopper on CP/M-86; sed chosen instead".
- [ ] Edit `prospects.md` sed row: update status to reflect active port underway.

**Relevant Context**  
- File: [`prospects.md`](prospects.md) lines 44–47 (section 3 Unix-culture text tools).

**Status** `[ ] pending`

---

## Sub-Task 2 — Replace `sed/Makefile` with Aztec-style build

**Intent**  
The existing `sed/Makefile` references a non-existent `${TOPSRC}/share/mk/sys.mk`
infrastructure (ELF, OBJDUMP, ELF2AOUT) — it has never been usable in this repo.
Replace it with the same pattern used by `grep`.

**Expected Outcomes**  
- `sed/Makefile` builds `sed.cmd` (CP/M-86) and `sed.com` (PC-DOS) using
  `aztec42_cc` / `aztec42_link`.
- `make clean` removes object files and binaries.
- No UPX step (sed is a two-file build; grep uses it, dc does not — follow dc's pattern
  given sed is larger, or add it later once size is known).

**Todo List**  
- [ ] Write new `sed/Makefile` modelled on `grep/Makefile`:
  - `CC=aztec42_cc`, `LD=aztec42_link`
  - `OBJS=sed0.o sed1.o`
  - targets: `binaries`, `sed.cmd`, `sed.com`, `clean`
  - `CPM_LDFLAGS=-lc86`, `DOS_LDFLAGS=-lc`
  - dependency lines: `sed0.o: sed0.c sed.h` and `sed1.o: sed1.c sed.h`

**Relevant Context**  
- Template: [`grep/Makefile`](grep/Makefile)
- Template: [`dc/Makefile`](dc/Makefile) (no UPX)
- Current broken file: [`sed/Makefile`](sed/Makefile)

**Status** `[ ] pending`

---

## Sub-Task 3 — Port `sed` source to Aztec CP/M-86

**Intent**  
Make `sed0.c` and `sed1.c` compile cleanly under `aztec42_cc` targeting CP/M-86.
Apply the CP/M-86 adaptations established as conventions in `grep` and `dc`.

### 3a — Header and library fixes (sed0.c + sed1.c + sed.h)

**Expected Outcomes**  
- All POSIX-only headers removed or guarded.
- `sed.h`: duplicate `reend`, `lbend`, `cp` declarations resolved (they appear twice: lines 34–41 and 97–99) — keep only one set.
- `sed1.c`: `#include <fcntl.h>` and `#include <unistd.h>` removed (Aztec C86 does not
  ship these POSIX headers); `open()`/`read()`/`close()` replaced with `fopen()`/`fread()`
  or equivalent standard-C equivalents available in Aztec's `c86` library.
  The `execute()` function in `sed1.c` currently uses low-level `open(file, 0)` /
  `read(f, ibuf, BUFSIZ)` / `close(f)` — replace with `FILE *` + `fread` or character
  reads so no POSIX fd layer is needed.
- Buffer sizes: `LBSIZE=4000` and `RESIZE=10000` in `sed.h` may need reduction if the
  combined static data overflows CP/M-86's 64 KB TPA. Reduce to `LBSIZE=1024` and
  `RESIZE=4096` as a first pass (same order as grep's 1024-byte line limit); adjust if
  tests show regressions.

**Todo List**  
- [ ] Remove `#include <fcntl.h>` and `#include <unistd.h>` from `sed1.c`.
- [ ] Replace `open(file, 0)` / `read(f, ibuf, BUFSIZ)` / `close(f)` in `execute()` with
  `fopen` / `fread` (or `fgets`) / `fclose`; update `int f` global to `FILE *f` (or a
  local variable), updating all references in `gline()` accordingly.
- [ ] Fix duplicate declarations in `sed.h` (remove second block of `cp`, `reend`, `lbend`
  at lines 97–99 and duplicate `depth` at line 149).
- [ ] Reduce `LBSIZE` to 1024 and `RESIZE` to 4096 in `sed.h`.
- [ ] Replace any `perror()` call with plain `fprintf(stderr, ...)` (Aztec c86 has no
  `sys_errlist`).

### 3b — Case sensitivity: option-letter folding

**Intent**  
CP/M-86 CCP folds the entire command tail to upper case before passing it to the program.
All option letters must therefore be recognised in both cases (or just upper case, since
lower will never arrive). Follow the grep convention: fold option chars to lower in the
switch before matching.

**Expected Outcomes**  
- `main()` in `sed0.c` option-parsing loop: `tolower(eargv[0][1])` (or a simple `| 0x20`
  for ASCII letter range) applied before the `switch`.
- Options `-n`, `-f`, `-e`, `-g` all work when typed as `-N`, `-F`, `-E`, `-G`.

**Todo List**  
- [ ] In `sed0.c` `main()`, fold the option character to lower case before the `switch`.
- [ ] Add `#include <ctype.h>` if `tolower` is used (Aztec c86 has `<ctype.h>`).

### 3c — `-o <outfile>` output-redirect option

**Intent**  
CP/M-86 has no pipes. `sed` output goes to stdout; without a pager or redirection the
screen fills up. Adding `-o <file>` lets the user redirect all output to a file.
This mirrors the same motivation as grep's `-p` (pause) option — a CP/M-specific
accommodation for the lack of pipes.

Implementation: redirect `stdout` via `freopen(outfile, "w", stdout)` early in `main()`
after argument parsing, before any output is produced.

**Expected Outcomes**  
- `-o outfile` (or `-O OUTFILE` — folded) opens `outfile` for write and connects it
  to `stdout` so all subsequent `putc(..., stdout)` / `fprintf(stdout, ...)` calls go
  to the file transparently.
- Error message and `exit(2)` if the file cannot be opened.

**Todo List**  
- [ ] Add `-o` case to the option switch in `sed0.c` `main()`: consume next `eargv`
  as the output file name.
- [ ] Call `freopen(outfile, "w", stdout)` after parsing; if it returns NULL, print
  an error and `exit(2)`.
- [ ] Document `-o` in the synopsis and options table (done in Sub-Task 5).

### 3d — Remove pipe / shell-escape code

**Intent**  
Search the source for any remaining shell-escape or pipe-creation code (the BSD tree
sometimes had `!` command or `popen`). There is none visible in the current source, but
confirm and guard.

**Expected Outcomes**  
- No `popen`, `system`, `fork`, `exec`, `pipe` calls remain.
- If any are found, remove or `#ifdef` them out.

**Todo List**  
- [ ] Grep source for `popen`, `system`, `fork`, `exec`, `pipe` and remove/guard any found.

**Relevant Context**  
- Files: [`sed/sed0.c`](sed/sed0.c), [`sed/sed1.c`](sed/sed1.c), [`sed/sed.h`](sed/sed.h)
- Pattern reference (case folding + freopen): [`grep/grep.c`](grep/grep.c) lines ~550–642
- Pattern reference (memmove→memcpy, header removal): [`dc/README.md`](dc/README.md) lines 102–109

**Status** `[ ] pending`

---

## Sub-Task 4 — Add `-o <outfile>` to `grep`

**Intent**  
`grep` already has `-p` (pause) as a CP/M-specific accommodation. Add `-o <outfile>` for
the same no-pipe reason: let the user capture grep output to a file instead of the screen.

**Expected Outcomes**  
- `grep -o result.txt pattern file` writes all matching lines to `result.txt`.
- `freopen` approach (same as sed): transparent to all existing output code.
- `-o` documented in `grep/README.md` options table and synopsis.
- Existing tests still pass.

**Todo List**  
- [ ] In `grep/grep.c`, locate the option-parsing loop.
- [ ] Add `-o` / `-O` case: consume next argument as filename, `freopen` stdout.
- [ ] Update `grep/README.md` synopsis line and options table.

**Relevant Context**  
- File: [`grep/grep.c`](grep/grep.c) — option loop around line 550
- Template README: [`grep/README.md`](grep/README.md)

**Status** `[ ] pending`

---

## Sub-Task 5 — Write `sed/README.md`

**Intent**  
Provide a high-quality user-facing README modelled exactly on `grep/README.md`:
synopsis, options table, "Case on CP/M-86" section, building instructions, porting notes,
bugs, see-also, license.

**Expected Outcomes**  
A `sed/README.md` covering:
- Synopsis: `sed [-ngo] [-e script] [-f file] [script] [file ...]`
- Options table: `-n`, `-e`, `-f`, `-g`, `-o`, plus CP/M-86-specific notes
- Case on CP/M-86 section: option letters are folded; the `%` toggle applies to
  regex patterns in `-e` scripts (since the CCP uppercases the command tail)
- Sed commands summary table (the standard `a c d D e g G h H i l n N p P q r s t w W x y =`)
- Building: `make` / `make clean`
- Porting notes (mirrors the findings in Sub-Task 3)
- Bugs section: line-length limit, reduced buffer sizes
- See also: `grep`(1)
- License: BSD 3-Clause

**Todo List**  
- [ ] Write `sed/README.md` using `grep/README.md` as structural template.
- [ ] Include all porting notes discovered in Sub-Task 3 (header changes, buffer sizes,
  case-folding, -o option, no-pipe rationale).

**Relevant Context**  
- Template: [`grep/README.md`](grep/README.md)

**Status** `[ ] pending`

---

## Sub-Task 6 — Update root `README.md` and root `Makefile`

**Intent**  
Add `sed` to the "Included projects" table in `README.md` and to the `SUBDIRS` list
in the root `Makefile` so it participates in `make all` and the CP/M disk image build.

**Expected Outcomes**  
- `sed` row added to the "Included projects" table (after `grep`).
- Root `Makefile` `SUBDIRS2` includes `sed`.

**Todo List**  
- [ ] Add `sed` row to [`README.md`](README.md) "Included projects" table:
  `| sed | Stream editor | RetroBSD / Unix V7 sed | BSD 3-Clause |`
- [ ] Add `sed` to [`Makefile`](Makefile) `SUBDIRS2`.

**Relevant Context**  
- File: [`README.md`](README.md) lines 41–62 (Included projects table)
- File: [`Makefile`](Makefile) line 3 (SUBDIRS2)

**Status** `[ ] pending`

---

## Execution Order

```
Sub-Task 1 (prospects.md)  →  independent, do first
Sub-Task 2 (Makefile)      →  independent, do after 1
Sub-Task 3 (port source)   →  depends on 2 (Makefile needed to verify build)
Sub-Task 4 (grep -o)       →  independent of 3
Sub-Task 5 (sed README)    →  depends on 3 (porting notes must be confirmed)
Sub-Task 6 (root files)    →  depends on 3 (sed must build cleanly first)
```
