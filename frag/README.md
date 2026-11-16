# FRAG

FRAG 1.16 (1985), by Charlie Godet-Ceraolo, based on Kelly Smith's
FILE-EXT (1982), itself derived from an earlier disassembly credited to J.A.P.

**Read-only:** lists CP/M file extents and allocation maps. It does not
rearrange, repair, defragment or write disk data. **DOS/FAT volumes are rejected**
before their synthetic directory entries can be interpreted as CP/M maps.

## Usage

```
FRAG
FRAG *.CMD
FRAG B:*.BIN
```

No filename means all files in the current user area. An explicit drive
selects that volume for probing and listing; the original drive is restored
on exit. Wildcards follow CP/M filename rules.

Output shows user number, filename, R/O/System/Archive attributes, the raw
EX and record-count bytes, and allocation entries. Numbers are hexadecimal.
Repeated filename text is suppressed for subsequent extents. This is a raw
inspection aid, not a fragmentation percentage or filesystem consistency check.

The selected drive's DPB determines allocation-entry width:

- DSM ≤ 255: sixteen one-byte block numbers, paired for an 80-column display.
  `0102 0304` means blocks 01, 02, 03, 04.
- DSM > 255: eight little-endian word entries, displayed as normal hexadecimal
  numbers. `0201` means block 0201, not blocks 01 and 02.

Zero allocation entries are blank. Up to 256 matching physical directory
entries fit; exceeding that limit reports an error without displaying a
partial map. Labels and timestamp entries are omitted.

## Build and tests

```
make
make test
```

Uses the existing ASM86/GENCMD toolchain. The separate data group reserves
10 KiB (`DATA[M280]`) for the initialized data, stack and 256-entry array;
GENCMD's inferred size alone omitted the uninitialized storage.

The local emu2 suite uses private diagnostic builds with a BDOS fixture shim.
It tests the actual scan, insertion and display routines with byte/word maps,
empty directories, FAT signatures, explicit drive selection/restoration, and
256/257-entry boundaries. It does not claim to emulate a real filesystem.

Real-OS checks are separate from make:

```
python3 testpce.py --work-dir /tmp/frag-check
```

The directory must be new. Requires PCE, cpmtools and mtools, using private
copies of the existing boot images. CP/M maps are compared with actual
on-disk entries; DOS/FAT must produce only the rejection diagnostic. Entire
test disk images are compared before/after to check that they remain unchanged.
Screens and monitor logs are retained. The default suite covers CP/M-86,
CCP/M-86 and CDOS 3.2/4.1; `--system` selects an individual OS.

Validated on 2026-11-16: all 12 fixture cases passed, and all six real-PCE
cases passed (four CP/M-volume listings plus FAT rejection on CDOS 3.2 and
4.1). All six data disks remained byte-for-byte unchanged. The real CP/M
fixtures include a multi-extent file and unsorted file creation order.
Word allocation maps and the 256-entry limit are covered by fixtures.

An exploratory `--system cdos62` run timed out during OS boot, before FRAG
executed; CDOS 6.2 compatibility remains unverified.

## Corrections

The original copied 34 bytes from a 32-byte directory entry and advanced
36 bytes through storage sized for 34-byte list nodes. The copy is now exactly
32 bytes plus its two-byte link, with an explicit capacity check. Allocation
width now follows DSM rather than always treating the map as words.

The raw-directory FAT header is recognized by its 20h user/header marker,
80h record byte and 8080h allocation sentinel, as used by the existing disk
browsers. No DOS interrupt, raw disk write or OS-version guess is involved.

## License

Original attribution remains in `frag.a86`. No explicit redistribution or
modification grant accompanied this historical source. The repository's
default license does not replace its original terms.
