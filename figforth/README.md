# fig-FORTH for CP/M-86

Native 8086 fig-FORTH 1.0, built as `FORTH.CMD`. The interpreter, compiler
and dictionary share one 64 KB CMD code group. There is no DOS build.

## Build and use

Run `make` with the cross-development tools on your path (`cpm86_asm86`,
`cpm86_gencmd`, `pcdev_cmdinfo`). Copy `FORTH.CMD` to a writable CP/M-86
drive/user area and run `FORTH`.

Use uppercase Forth words:

```forth
: SQUARE DUP * ;
12 SQUARE .
```

The result is `144`. `VLIST` lists the dictionary. `BYE` saves dirty block
buffers and returns to CP/M-86. This is historical fig-FORTH, not ANS Forth;
for example, use `0 VARIABLE VALUE` to create an initialized variable.

## Saving source

`FORTH.BLK` is opened or created on demand in the current drive/user area.
It is a normal CP/M file; the program never accesses raw disk sectors.

A `BLOCK` is 128 bytes. A source screen is 1024 bytes (eight blocks,
sixteen 64-character lines). `1 LIST` displays screen 1; `1 LOAD` interprets
it. New file gaps are explicitly filled with spaces; reads beyond EOF also
return spaces. The file supports blocks 0–8191
(screens 0–1023); reserve screen 0 and start source at screen 1 because
fig-FORTH uses zero `BLK` for terminal input.

For a small persistent example:

```forth
65 8 BLOCK C! UPDATE
FLUSH
BYE
```

After restarting, `8 BLOCK C@ .` prints `65`. Call `UPDATE` after modifying
a block and `FLUSH` to commit pending changes. `BYE` also flushes them.
A storage error reports `FORTH.BLK I/O error` and exits; failed writes are
not silently accepted. Back up the block file as you would any source file.

Definitions typed at the prompt are held in memory only. Save source in
screens and reload it; this port does not yet provide a dictionary-image
save command or a bundled screen editor. A host can create a source screen
by padding each of its 16 lines to 64 ASCII bytes with spaces (no CR/LF).

## Implementation and provenance

- `forth.a86`: historical Forth core adapted to Digital Research ASM86.
- `cpmio.a86`: CP/M-86 console, printer and random-record file I/O. Keyboard input
  uses BIOS `CONIN` through BDOS 50, as in the original CP/M-86 listing.
- `tests/test.py`: native-binary regression tests using emu2 and private directories.

The [March 1981 FIG listing](https://www.forth.org/fig-forth/fig-forth_8086-8088_ver_10.pdf)
is Thomas Newman's original CP/M-86 implementation. The machine-readable
starting point here is Charlie Krajewski's later IBM-PC adaptation from the
[FIG archive](https://www.forth.org/library/eforth_SOC/eforth_SOC_source/figforth/FIGFORTH.ZIP).
This is not a byte-for-byte reconstruction of the PDF.

The port replaces DOS/BIOS calls and raw disk access with BDOS, removes the
DOS interrupt-vector patch, reserves space for the adapter before the
extensible dictionary, and supplies file-backed blocks and a flushing `BYE`.
The original credit notices and adapter terms are in [LICENSE.md](LICENSE.md).
The unmodified source archive is in `upstream/figforth.zip` for comparison.

## Examples

Five short examples and their expected output are in
[examples/README.md](examples/README.md). `make` packs them into
`examples/demo.blk` for loading on CP/M-86.

## Tests

See [tests/README.md](tests/README.md) for the test layout.

`make test` runs arithmetic, stack manipulation, definitions, loops,
conditionals, variables, defining words (`DOES>`), `FORGET`, unknown-word
recovery, blank-block reads,
write/reopen persistence, extent boundaries, automatic `BYE` flushing,
source-screen loading and block bounds checks. It runs the 8086 CPU model
with emu2's 2.2 and 3.1 BDOS profiles; these profiles are not real OS boots.
Set `EMU2=/path/to/emu2` to select a particular emulator build.

PCE checks are separate from `make test`: boot CP/M-86 with a private data
disk, run the `SQUARE` example, then the save/restart example above. No test
should be run against a valuable `FORTH.BLK` file.

Verified on CP/M-86 1.1 under PCE: compiling `SQUARE`, evaluating
`12 SQUARE .`, returning with `BYE`, and reopening the block file in a new
process. The saved byte and blank-filled preceding records were also checked
in the resulting disk image. Physical hardware has not been tested.
