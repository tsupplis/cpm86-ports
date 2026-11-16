# DU — disk utility 7.5-D

DU inspects and edits raw CP/M disk records, displays allocation maps and finds
file data. Unlike FRAG, it can write sectors, including directory and system
tracks. Work on a disposable copy of a disk first; writes bypass file protection.

## Build and test

```sh
make
make test
```

Builds `du.cmd` with ASM86 and GENCMD in the 8080 memory model, reserving 60 KiB
for code, buffers, stack and directory workspace. No external runtime is needed.

The test target runs 19 source-derived assembly diagnostics under emu2 with a
fake BIOS: read failures and retries, write gating, cache reuse, record preservation,
version selection and DOS-DPB rejection. It does not access real disk sectors.
Production BIOS/XIOS testing passes on CP/M-86 1.1, CCP/M-86 3.1 and Concurrent
DOS 4.1: read two records, change one byte, write, switch physical sectors and
reread. Whole-image comparison after shutdown verifies that only the intended
byte changed. Concurrent DOS also rejects a DOS disk without changing it.

Run these four scenarios separately from the Makefile:

```sh
python3 testpce.py --work-dir /tmp/du-pce-check
```

The work directory must not already exist. The test copies the existing PCE boot
images/configurations and prepares disposable data disks. It prints the tested
CMD's SHA-256 and measures each OS's BDOS identifier with a diagnostic companion.
The companion does not replace or modify DU. `--system cpm86`, `ccpm86` or
`cdos41` selects one system; `--probe-only` only measures BDOS versions.
PCE, cpmtools and mtools are required for this separate validation.

## Usage

Run `DU`, then `?` for the built-in command reference. Commands may also follow
`DU` on the command line. Separate multiple commands with `!`.

| Command | Action |
| --- | --- |
| `LB` | Select drive B |
| `#` | Display disk parameters |
| `T2!S1!R!D` | Select track 2, logical sector 1, read and dump |
| `+` / `-` | Move forward / backward a sector |
| `D`, `H`, `A` | Hex/ASCII, hex-only, ASCII-only display |
| `FNAME.EXT` | Find a file |
| `M` | Allocation map |
| `CH0,41` | Change byte 0 of the buffered record to hex 41 |
| `W` | Write the buffered record to disk |
| `X` | Quit, with confirmation |

Track and sector numbers are decimal (`T30`, `S2`). Change offsets and byte
values are hexadecimal (`CH0,5A`). Disk records are 128 bytes;
on newer systems DU preserves the other records in the physical sector.
A successful read is required before writing. After a read/write error, read
again successfully before attempting another write.

## Compatibility and hardening

The version gate accepts BDOS identifiers `0022h` (IBM CP/M-86 1.1),
`1022h` (the emu2 CP/M-86 profile), `1031h`
(CP/M-86 Plus), `1431h` (CCP/M-86) and `1441h` (Concurrent DOS 4.1).
The `0022h`, `1431h` and `1441h` systems passed the PCE scenarios above.
CP/M-86 Plus (`1031h`) remains unvalidated; its legacy backend is retained.
Fault injection is covered by assembly diagnostics, not physical-media failures.
Other versions, MP/M and DOS-format DPBs are rejected. Physical sectors above
2048 bytes are rejected by the newer-system backend.

Failed reads do not populate the physical-sector cache or replace caller data.
A prerequisite read failure aborts a physical write; failed writes invalidate the
cache and revoke write permission. DOS-DPB rejection occurs before DU changes
the drive's login-sequence field. For accepted CP/M drives the legacy backend
still clears that field to force a later disk reset to log in permanent media.

The supplied source references `DU77.DOC`, which was not supplied. The embedded
`?` help is the available original command documentation.

## Origin

Based on Ward Christensen's DU 7.5 (1981), translated to CP/M-86 by an unnamed
contributor and extended by H. M. Van Tassell. This source identifies revision
7.5-D dated 29 March 1985. See [LICENSE.md](LICENSE.md).
