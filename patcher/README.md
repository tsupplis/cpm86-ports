# PATCHER

PATCHER 1.0 (8-Jan-2001) builds a dedicated binary patch installer for
CP/M-86. Filenames, offsets and old/new byte pairs are compiled into the
program; it does not accept patch descriptions on the command line.
Public domain; see [LICENSE.md](LICENSE.md).

## Build and use

```
make
make test
```

The Makefile follows the single-source ASM86 projects: `cpm86_asm86`,
`cpm86_gencmd` and `pcdev_cmdinfo`, with the original 8080-model
`CODE[M200]` allocation. The shipped template has `files db 0` and changes
nothing. The retained six-file tables are placeholders, not usable patches.

Edit the marked configuration area in `patcher.a86`:

1. Set `hello` to describe the patch and `files` to the file count.
2. In `fname`, supply uppercase bare 8.3 filenames, each preceded by
   `0` (required) or `1` (optional) and followed by a zero byte.
   Drive prefixes, paths and wildcards are not supported.
3. In `bytes`, give the number of patch locations for each file; use
   positive counts. Keep all tables in the same file/location order.
4. In `offs`, give each absolute byte offset as **high word, low word**.
   For example, byte `14259h` is `dw 1,4259h`. Use offsets below
   `80000000h`, within the existing file and the OS-supported file size.
5. In `value`, give the expected and replacement byte for every location.
   Use each offset once. Ensure table counts agree before building.

For example, one patch changing byte 128 of `APP.CMD` from `74h` to `75h`:

```asm
files   db 1
fname   db 0,'APP.CMD',0
bytes   dw 1
offs    dw 0,80h
value   db 74h,75h
```

Rebuild with `make`, copy `PATCHER.CMD` to the target disk, select the
user area containing the target files, and run `PATCHER`. Back up targets
first and keep other applications from modifying them during the run.
The first pass checks all configured locations before the second pass
writes. A location already containing its replacement byte is accepted;
this checks patch sites, not the identity of the entire file.

Patching is **in place**, with no rollback: an I/O failure during the write
pass can leave some changes applied. Read the final messages; this CP/M
version does not provide the DOS ERRORLEVEL interface. Optional missing
files generate warnings. A repeat run accepts already-patched bytes.

## Compatibility and inspection

Uses 8086 instructions and standard CP/M-86 BDOS console, open/close and
random-record file operations. There is no BIOS layout, UART, video or
terminal dependency. CP/M-86 and CCP/M-86 are intended targets; this is
not a DOS executable. Universal compatibility is not established by an
emulator version profile.

Local corrections zero the FCB control fields before opening a file and
report close failures instead of claiming success. The default configuration
is disabled to make the distributed template harmless.

`make test` builds a private configured copy from the current engine and
checks preflight rejection without modifications, exact multi-file patching
at offsets 0, 127, 128 and 65537, and repeat execution, using emu2 profiles
2.2 and 3.1. These are emulator tests, not real-OS certification. Close-error
handling still needs an OS or fault-injection test. No PCE is invoked by make.

## Original PATCHER 1.0 documentation

The original manual is reproduced below, including its DOS instructions
and references to separate tools; those DOS sources are not supplied here.

```text
PATCHER 1.0
-----------
PATCHER is a generic file patching program in source form for MSDOS and
CP/M-86.  It provides a simple way of providing end-users with patches
where distributing the revised binary is not feasable.

PATCHER is useful when simple one-for-one byte patches are needed.  It
is less suitable for those applications where the difference between
the source and target files is great.

Features
--------
 - Patches multiple files.
 - Verifies all target files for correct version before any patches are
   applied.
 - Target files may be selectively marked as 'optional' allowing patching
   to occur even if some files were not found.  This allows patching of
   applications which are installed with some components missing.
 - The MSDOS version of PATCHER returns ERRORLEVEL for batch processing
   (0=success, 1=error).

Instructions
------------
You will need the following data:

 - The name of file(s) to be patched.
 - The offset from the start of the file where each patch is to be made.
 - The old and new values for each byte patched.

Edit PATCHER.ASM (or PATCHER.A86 for CP/M-86) using any text editor.  It
is only necessary to change the data in the boxes marked with asterix!

Finally, assemble using any 8086 assembler e.g.

For MSDOS:
 
   A86 PATCHER.ASM     (if using A86)

   TASM PATCHER        (if using Borland's TASM)
   TLINK /T PATCHER

For CP/M-86 (if using DRI ASM86):

   ASM86 PATCHER
   GENCMD PATCHER 8080 CODE[ M200]


Disclaimer
----------
PATCHER is public domain.  Use of this program is at your own risk and
responsibility.


Addendum
--------
A binary file compare utility can be a handy adjunct to PATCHER.  Some
suitable programs are listed below.
 
1. The MSDOS program FC.EXE can be used to generate the differences
   between two binary files. e.g.

     FC /b oldfile newfile >diff.txt

   DIFF.TXT then provides the address offsets and difference bytes for
   cutting/pasting into PATCH.ASM.  A 'column mode' editor e.g. UltraEdit
   will simplify the process and minimize transcription errors.

2. For CP/M-86 users, there is BINCOM 2.0 (7-Jan-01).  This program works
   in a similar manner to FC.EXE and has a 'log to disk' option.
```
