# ARC

`arc86.cmd` and `marc86.cmd` are the Unix ARC 5.21 archiver and archive merger
(Howard Chu, based on Thom Henderson's MSDOS ARC) ported to CP/M-86 with
Aztec C86 (`aztec43_acc`, small model).

The archive format is the standard ARC 5.xx format. Archives are
interchangeable with `ARC86.CMD` (D. J. Weatherall's CP/M-86 port of
ARCCPM), see [Compatibility with ARC86](#compatibility-with-arc86).

## Synopsis

```
arc86 {amufdxeplvtcr}[bswnoqg<passw>] <archive> [<filename> ...]
marc86 <tgtarc> <srcarc> [<filename> ...]
```

The CP/M-86 CCP folds the command tail to upper case, so commands and options
may be given in either case. The archive extension defaults to `.ARC`.

## Commands

| Command | Meaning |
| --- | --- |
| `a` | Add files to the archive |
| `m` | Move files to the archive (sources are deleted) |
| `u` | Update files in the archive (newer files only) |
| `f` | Freshen files already in the archive |
| `d` | Delete files from the archive |
| `x`, `e` | Extract files from the archive |
| `p` | Copy files from the archive to standard output |
| `l` | List files in the archive |
| `v` | Verbose listing |
| `t` | Test archive integrity |
| `c` | Convert entries to a new packing method |
| `r` | Run files from the archive: **not supported** on CP/M-86 |

## Options

| Option | Meaning |
| --- | --- |
| `b` | Retain a backup copy of the archive (`.BAK`) |
| `s` | Suppress compression (store only) |
| `w` | Suppress warning messages |
| `n` | Suppress notes and comments |
| `o` | Overwrite existing files when extracting |
| `g<passw>` | Encrypt or decrypt archive entries with a password (must be last) |
| `q` | Squash instead of crunching: **not supported** on CP/M-86 |

## CP/M-86 port

The port keeps the portable ARC sources and adds a `__CPM86__` configuration
in [`arc.h`](arc.h) and a few `#if __CPM86__` blocks.

| Area | Behaviour on CP/M-86 |
| --- | --- |
| File lists and wildcards | `dirent_load()` from `util.lib` (`dirent.h`); the template may carry a drive, for example `B:*.DAT` |
| File dates | Read with `getfdatetime()` from `util.lib` when the system supports file stamps; otherwise the system clock (`time()`) is used. Stamps before 1980 are clamped to 1980-01-01 |
| Setting dates on extract | `setfdatetime()`; failure is accepted and reported once per run (`w` silences it) |
| Directories and user areas | None: CP/M-86 has drives only. Names are `[d:]name.ext` |
| Environment | None: `ARCTEMP` and `TMPDIR` are not used, temporary files go to the current drive |
| Compression | Crunch (12 bit), pack, squeeze and store. Squashed entries (13 bit) need more memory than the 64K data segment gives and are refused with a message |
| Buffers | 2 KB file buffers (`MYBUF`) |
| File sizes | CP/M-86 stores sizes in 128 byte records: an extracted text file is padded with NULs up to the next record boundary, and the header length includes the padding |
| `r` command | Not available (no `system()`) |
| File mode | Aztec stdio is always binary, so `"r"` and `"w"` are used (a `"b"` in the mode makes `fopen` fail) |

The library calls are used exactly as declared in `util.h` and `dirent.h`;
`util.lib` is a binary supplied with the repository.

## Building

```sh
make          # arc86.cmd and marc86.cmd
make test     # round-trip tests under emu2
make clean
```

`util.lib`, `util.h` and `dirent.h` in this directory are links to `../util/`.

## Testing

`make test` runs [`test.sh`](test.sh) under `emu2` and checks: add, list,
integrity test, extract of all files and of a single file, wildcard add,
delete, store-only, overwrite on extract, refusal of squash, and `marc86`
merging. Extracted files are compared as a prefix of the original with only
NUL record padding allowed after it.

## Compatibility with ARC86

[`legacy/arc86.cmd`](legacy/arc86.cmd) is D. J. Weatherall's 1987 CP/M-86 port of
Reimer Mellin's ARCCPM ([`legacy/arc86.md`](legacy/arc86.md)).

### Archive format

| Check | Result |
| --- | --- |
| Archive written by this ARC read by `ARC86` | Extracts correctly |
| Archive written by `ARC86` read by this ARC | Extracts and tests clean (`No errors detected`) |
| Same three files archived by each tool | Archives are 2,816 bytes each; entry names, order, header version, crunched sizes, lengths (with 128 byte padding) and CRCs are identical |
| Byte comparison of those archives | Differ only in the entry time stamp (seconds); with the stamp equal the archives are identical |
| Entry types exercised | Crunched, packed, stored; text, binary and random data |

### Command line

| Feature | ARC86 | This ARC |
| --- | --- | --- |
| `a`, `m`, `x`, `e`, `d`, `l`, `v` | Yes | Yes |
| Options `b`, `n`, `w`, `s`, `g<passw>` | Yes | Yes |
| `t` | Set the entry date and time (prompts) | Test archive integrity |
| `z` (extended BIOS date, DEC Rainbow) | Yes | Not accepted |
| `u`, `f`, `p`, `c`, `o` | No | Yes |
| Number of files on one `a` command | In testing only the first three names were added | No limit |
| Dates | Entered with `t`, or from the system clock | File stamp when available, else system clock |
| Messages and listing layout | ARCCPM style | ARC 5.21 style |

Known ARC86 quirks, which this ARC does not reproduce:

* The documented "I/O error 91" when a file is added after a shorter one
  (adding the 6000 byte file `LONG.DAT` on its own aborted `ARC86` in testing).
* A list may report a bad header after such an abort.

### Not covered

Squashed (`q`) entries, the `z` option, password-protected entries
cross-checked between the two tools, and file stamps read from a system that
supports them.

---

## Original notes

### Notes for ARC 5.21e, October 31, 1991

ARC 5.21 for Unix is at patch level 6. The code was used on 4.3 BSD based Unix
systems and on the Atari ST (GNU C 1.40; it should still compile with Mark
Williams C). It is over twice as fast as previous releases for all file
compression and decompression operations, and fixes longstanding bugs in the
squash code and in the `tmclock` DST code.

The `Arcinfo` file describes the ARC 5.xx directory format. The 6.xx format
contains additional information that is not supported here.

System V support in the shar file `Sysvarcstuf` was provided by Jon Zeeff,
Janet Walz and Rich Salz. You will probably also need the dirent directory
library written by Doug Gwyn.

The `tmclock.c` file was taken from Jef Poskanzer's tws library, as
distributed in PHOON, and slightly modified. The man page was originally from
Leo Wilson, with updates by Howard Chu.

### Notes for ARC 5.21, June 6, 1988

This program is based on the MSDOS ARC program, version 5.21, plus a few
enhancements.

* ARC also performs Huffman squeezing on data. The squeeze algorithm was
  removed from MSDOS ARC after version 5.12, and is more efficient than
  Lempel-Ziv style compression for graphic images. Squeeze analysis is always
  done now, and the best of packing, squeezing or crunching is used.
* ARC compresses and extracts squashed files. Squashing was created by Phil
  Katz in his PKxxx series of ARC utilities for MSDOS. Dan Lanciani wrote the
  original modifications to ARC's crunch code to handle squashing. The `q`
  option squashes files instead of crunching them, and the result is compared
  against packing and squeezing as before.

System specific notes:

* **MTS:** an additional `i` option ("image mode") was used. ARC assumes files
  are text by default and translates EBCDIC to ASCII before storing in an
  archive, and back on extraction. `i` inhibits this translation. The `r`
  command is omitted, and files cannot be restored with their original time
  stamps.
* **Unix:** the `i` option is also present. Unix ARC assumes a binary file by
  default; the only translation is end-of-line processing. With `i`, `\n` is
  changed to `\r\n` when storing and back when extracting.
* **Atari ST:** the `h` ("hold screen") option delays exiting the program, for
  use from the desktop. It is always active for MARC.
* **Unix and Atari:** ARC and MARC search for the environment variables
  `ARCTEMP` or `TMPDIR` and create temporary files in that directory.

The enclosed documentation is taken directly from the MSDOS distribution of
ARC. Unless specified differently, the programs behave identically. `ARC521.DOC`
is identical to `ARC520.DOC`; the differences between the two versions are in
`CHANGES.521` (here `arc531.doc` and `Changelog`).

Howard Chu, `hyc@umix.cc.umich.edu`, University of Michigan (1988) and Jet
Propulsion Laboratory (1991).

## License

GNU General Public License, see [`COPYING`](COPYING) and [`LICENSE.md`](LICENSE.md).
Thom Henderson, the original author of ARC, agreed to a free license on 15 Sep
2003, and Howard Chu, the maintainer of the Unix version, released it under the
GPL on 7 Oct 2003.
