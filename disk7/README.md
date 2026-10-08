Disk 7
=======

`disk77b` (DISK7 7.7) — a full-screen file manager: browse a directory one
file at a time and copy, delete, rename, view, print or tag files.

DISK7 by Frank Gaudé (1984), for CP/M-80, translated to CP/M-86 with XLT86.

## Synopsis

```
disk77b [d:][filename.typ]
```

With no argument all files on the current drive are listed. Wildcards
select a subset, e.g. `disk77b b:*.asm`. Press any key that isn't a command
to get the menu back.

## Commands

| Key | Action |
| --- | --- |
| `<SP>`, `<CR>` / `B` | Next / previous file |
| `C` | Copy the file to another drive/user, with CRC check |
| `M` | Copy all tagged files to another drive/user |
| `T` / `U` | Tag / untag the file |
| `D` | Delete the file |
| `R` | Rename the file |
| `L` | Show the file length |
| `V` | View a text file (`^C` cancels, `<SP>` one line, other keys a page) |
| `P` | Print a text file |
| `G` | Go to a filename (wildcards allowed) |
| `J` | Jump forward 22 files |
| `N` | New directory: log in another drive/user |
| `S` | Free space on a drive |
| `X` | Exit to CP/M |

A drive/user is typed as `d`, `d:`, `dn` or `dnn`, with an optional colon,
e.g. `B3:`.

## Building

```sh
make          # disk77b.cmd (CP/M-86, 8080 model)
make clean
```

The original DR MAC source is kept unchanged in `disk77b.mac`. The build
turns it into CP/M-86 code in four steps:

1. `prep80.py` makes plain 8080 source that XLT86 accepts (`disk77b.asm`).
   It expands the Z80-style jump macros (`JR`, `JRZ`, `DJNZ`, ...) into the
   8080 code they generate, rewrites `ELSE` (DR's tools have none), moves
   labels off `ORG` and `END`, renames symbols that are ASM86 reserved words
   or 8086 instructions (`TYPE`, `LIST`, `LAST`, `LOOP`, `ESC`), and splits
   long `DB` strings, which XLT86 would cut.
2. XLT86 translates it to 8086 (`disk77b.a86`).
3. `orig.a86.patch` applies the CP/M-86 fixes a translator can't make:
   - set SS to the program's own segment before switching stacks;
   - exit through BDOS 0, not a near `RET`;
   - read the console with BDOS 6, as there is no BIOS jump table at
     `0001h`;
   - read the results of BDOS 27 and 31 through ES, where CP/M-86 returns
     them.
4. ASM86 and GENCMD build `disk7.cmd` in the 8080 model, with 16K to 64K
   of memory for the filename list and the copy buffer.

See https://github.com/tsupplis/cpm86-crossdev for the cross-development
environment.

## License

Copyright (c) 1984 Frank Gaudé, all rights reserved. Free for non-commercial
use: monetary gain is not permitted without written permission from the
copyright assignee, Echelon, Inc.
