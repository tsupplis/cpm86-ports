# SD — Super Directory

Sorted directory listings for CP/M-86, with file sizes, free space, user-area
selection, file-type sorting and optional `.LBR` member listings. This is the
SD-derived **D 4.8C2** source, brought in as `SD.CMD`. It credits H. M. Van
Tassell, Bruce R. Ratoff and Keith Petersen. The historical [sd.doc](sd.doc)
is retained; this README describes the current port.

## Build and usage

```
make
```

Uses the established `cpm86_asm86`, `cpm86_gencmd` and `pcdev_cmdinfo` tools.
The CMD uses the 8080 memory model (8086 instructions, shared code/data segment)
with a minimum allocation of approximately 16 KiB.

```
SD
SD B:*.CMD [NS
SD B:*.CMD [NS]
SD [TN]
SD B: [U3N]
SD *.LBR [LN]
SD *.CMD [FN]
SD //
```

Options start with `[`. The closing `]` is optional. Letters can be grouped,
comma-separated or space-separated: `[NS`, `[NS]`, `[N,S]` and `[N S]` are
equivalent. No new option letters have been added.

| Option | Meaning |
| --- | --- |
| A | All user areas, starting at 0 unless U/G supplies a starting user |
| D | Search drives in the OS login vector, starting at A: or the explicit drive |
| F | Append the listing to `SD.DIR` on the original drive/user area |
| L | Include library member listings |
| N | Disable page pauses |
| P | Echo output to the printer |
| R | Reset the disk system before searching |
| S | Include system files |
| T | Sort by extension, then filename |
| U*n* / G*n* | Select user 0–15 |
| V | Display the version before the listing |

`D` searches **logged-in drives**, avoiding guesses about installed drives and
the old machine-specific C:–L: skip. Select a drive first if it is not logged in.
Repeated option letters retain the original error behavior. File sizes are
allocated kilobytes rounded to the disk's allocation-block size.

## Quality corrections

- Bounds-check directory-table arithmetic and every entry before writing;
  reserve space below the runtime stack. Reject invalid directory-search slots.
- Preserve high extent bits so files above 512 KiB sort and size correctly.
- Check the returned record before recognizing CDOS's synthetic FAT header;
  failed searches cannot reuse stale data as a header.
- Check library open/read results, including truncated library directories.
- Check output close errors. At an exact 16 KiB extent boundary, inspect the
  previous record's Ctrl-Z before appending. Existing contents are preserved.
- Restore the caller's drive and user area on normal/error exits.
- Print five-digit decimal quantities correctly.

The historical append algorithm addresses 32 logical extents. It now rejects
an existing `SD.DIR` of 512 KiB or larger rather than wrapping the extent field;
archive or rename that listing before collecting more output. Totals/free-space
counters remain 16-bit kilobyte values and are not suitable for reporting disks
or aggregate sizes of 64 MiB or more.

## Tests

```
make test
python3 testpce.py --work-dir /tmp/sd-pce
```

The local suite runs the production CMD through emu2 with private files, plus
source-derived diagnostic builds for capacity, arithmetic overflow, search and
close errors, FAT detection and drive/user restoration. Each run has a ten-second
timeout. Diagnostic BDOS fixtures are not real OS compatibility tests.

The standalone PCE test boots private CP/M-86, CCP/M-86 and CDOS 4.1 images,
exercises both bracket forms, then extracts `SD.DIR` and checks that two appends
preserve its original 16,383-byte prefix. PCE is deliberately outside the
Makefile. The work directory must be new; images and logs remain there.

Existing-file append is tested in PCE: emu2's file-open delegation uses the DOS
FCB layout and does not provide the CP/M record count required by this algorithm.
No emulator modifications or application workarounds are included.

See [LICENSE.md](LICENSE.md) for the available licensing evidence.
