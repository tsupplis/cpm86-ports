wash15
======

`wash15` (WASH 1.5) — a directory maintenance utility: step through the
files of a drive in alphabetical order and view, print, copy, rename or
delete them.

WASH by Michael J. Karas (Micro Resources, 1981), version 1.5 by Simon J.
Ewins, for CP/M-80, translated to CP/M-86 with XLT86. See
[wash15.doc](wash15.doc) for the original documentation.

## Synopsis

```
wash15 [d:][filename.typ]
```

With no argument all files on the current drive are listed. Wildcards
select a subset, e.g. `wash15 b:*.asm`. The list is circular: it wraps
round at both ends.

## Commands

| Key | Action |
| --- | --- |
| `<SP>`, `<CR>` | Next file |
| `B` | Previous file |
| `V` | View the file on the console (any key aborts) |
| `L` | Print the file on the list device |
| `P` | Send the file to the punch device |
| `C` | Copy the file to another drive |
| `R` | Rename the file |
| `^D` | Delete the file |
| `S` | Start again on another drive |
| `H` | Show the command list |
| `X` | Exit to CP/M |

## Building

```sh
make          # wash15.cmd (CP/M-86, 8080 model) from wash15.a86
make test     # allocation bounds and copy regressions under emu2
make clean
```

`wash15.a86` is the translated, fixed source and the one to edit. It is
built from the original 8080 source, kept unchanged in `orig.src`, with
`make orig.a86 && cp orig.a86 wash15.a86`:

1. Renames a few symbols into `orig.asm`. XLT86 stops with
   `Fatal Error: Not BDOS` on calls to a routine named `CDEHL`: the name,
   not the code, trips it, so it becomes `CMPDEHL`. `LIST`, `LOOP` and
   `ESC` clash with ASM86 reserved words and 8086 instructions.
2. Translates it to 8086 with XLT86.
3. Applies `orig.a86.patch`, the CP/M-86 fixes a translator can't make:
   - set SS to the program's own segment before switching stacks, and turn
     interrupts back on (the original disables them and never re-enables);
   - get the logged drive from BDOS 25, as CP/M-86 has no drive byte at
     `0004h`;
   - read and write the `PNAME` buffer, which sits inline in the code,
     through `DI`: ASM86 won't use a code label as a byte variable.

`wash15.cmd` is built with 16K to 64K of memory for the file list and the
copy buffer. `wash15.com` is the original CP/M-80 program.

The file list and copy buffer use the actual CP/M-86 allocation recorded in
the base page, not the CP/M-80 BDOS entry address. Each list append leaves
room for at least one 128-byte copy record. A directory that cannot fit is
reported as `Directory too large for memory`, and the program exits without
performing file operations or presenting a partial list.

The tests use disposable CMD copies with smaller base-page allocations,
check canaries beyond the memory limit, and copy across multiple buffers.
The shipped CMD is also checked with its normal startup and menu.

When copying, only BDOS read status 1 is treated as EOF. Other read errors
abort the copy, close the files, and delete the incomplete destination.
Cleanup failures are reported explicitly. Tests inject read errors before
the first record and after records have already been written.

See https://github.com/tsupplis/cpm86-crossdev for the cross-development
environment.

## License

Released to the public domain by Michael J. Karas. No commercial use: the
program may not be sold, in whole or in part, modified or not.

## SYSTEM and FAT HIDDEN browsing

The default list contains normal files. `O` toggles normal/all, including
SYSTEM and FAT HIDDEN files. S/H indicators distinguish those attributes;
an empty filtered list offers the same toggle instead of sorting an empty
ring. This normal-only default also applies on CP/M and CCP/M.

Attributes occupy a separate byte in each in-memory record, travelling with
the filename through sorting and deletion. They are never encoded in CP/M
filename bits. A version gate followed by the raw FAT header signature is
required before interpreting FAT metadata; CP/M allocation bytes remain
allocation data. No BDOS 230 or DOS interrupt is used.

Rename and deletion still use BDOS 23 and 19. READ-ONLY/locks remain enforced
by the OS; browsing a HIDDEN file does not clear its attributes. The updated
CDOS HIDDEN-deletion backend is required to delete HIDDEN files on FAT.

Build with `make`. The opt-in integration test (from the ports root) is:

```
python3 tools/hidden_browser_check.py wash15 --boot BOOT.img --config PCE.cfg
```

It uses disposable images and one emulator, checking normal/all/empty lists,
rename/delete, attribute retention and untouched labels/directories.
