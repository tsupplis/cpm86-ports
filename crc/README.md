crc
===

`crc` — checksum files.

This is a 32-bit CRC calculator ported to CP/M-86 (and PC-DOS) using the Aztec C86 4.2 cross compiler.

## Synopsis

```
crc [-po] [file ...]
```

## Options

| Option | Meaning |
| --- | --- |
| `-o` | Write all output to *file* instead of the screen; CP/M-86 has no pipes. Incompatible with `-p` — if both are given, `-o` wins and `-p` is ignored. |
| `-p` | Pause after 23 lines of output until a key is pressed (`^C` quits); CP/M-86 has no pipes to a pager. Ignored if `-o` is also given. |

Option letters may be given in either case, because the CP/M-86 CCP folds the command tail: `-O` and `-o` both mean `-o`.

## Description

For each file, _crc_ calculates and prints a 32-bit CRC, the byte count, and the file name. It is typically used to validate files transferred between different systems, and is useful in detecting subtle disk corruption.

The 32-bit CRC used is the frame check sequence in ADCCP (ANSI X3.66, also known as FIPS PUB 71 and FED-STD-1003, the U.S. versions of CCITT's X.25 link-level protocol). This polynomial is compatible with the DOS version of `crc`, the 32-bit CRC used by PKZIP version 0.9, as well as the `crc` command in ZCOMM and Professional-YAM.

## Building

```sh
make          # crc.cmd (CP/M-86) and crc.com (PC-DOS)
make clean
```

The `-p` pause option reads its key through BDOS direct console I/O (function 6).

## License

BSD 3-Clause / Public Domain. Copyright (C) 1986 Gary S. Brown.
