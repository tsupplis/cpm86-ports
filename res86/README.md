# RES86

Interactive 8086 disassembler for CP/M-86, based on RESOURCE by Ward
Christensen, translated by Larry Etienne and extended by W. Earnest.
This is version 2.4 (17 April 1985), with local loader and annotation hardening.
`res86.a86` includes `res86b.a86`; they are two parts of the same program.
The original manual is retained in [res86.doc](res86.doc).

## Build and use

```
make
```

Requires `cpm86_asm86`, `cpm86_gencmd` and `pcdev_cmdinfo`. The executable
reserves a 63.75 KiB data group for its tables, in addition to code and the
image being inspected. Loading a large image therefore needs additional RAM.

Start `RES86` at the CP/M prompt, then enter commands at `C->`:

```
RDEMO.CMD       load a CMD (falls back to DEMO.COM if absent)
L0,40          disassemble a hexadecimal address range
D0,7F          hexadecimal/ASCII dump
DDS:0,7F       dump the data segment
E10,.START     name address 0010 START
;10,;ENTRY     add an appended comment
C20,B          mark address 0020 as byte data
SDEMO.SYM      save symbols
SDEMO.DOC      save comments
SDEMO.CTL      save code/data classifications
RDEMO.ALL      reload image and annotations
SDEMO.A86      start capturing assembly output
L0,40          emit the desired listing
Z              finish the assembly output file
H              command summary
?              table and segment statistics
```

Addresses are hexadecimal. A leading dot identifies a symbol in commands.
At the command prompt, Ctrl-C returns through the OS console service; during
listing, Ctrl-C invokes RES86's `ABORT Y/N` prompt. `X` purges tables; it is
**not** an exit command. See the original manual for the complete syntax.

## Hardening and limits

- CMD loading checks all eight descriptor slots, group types, duplicates,
  presence of stored code, individual 64 KiB limits and truncated bodies.
  Groups are contiguous in the file; only the final 128-byte record is padded.
  Bytes beyond the declared image (such as relocation information) are ignored.
- COM loading reserves the 100h origin and rejects empty files or files above
  510 records (65,280 stored bytes). Reloads clear stale segment addresses.
- Symbol, comment and control imports and interactive insertions are bounded.
  Strings have a maximum of 255 bytes; control tables hold 767 entries plus
  their terminator. Comments and controls cannot use the FFFF sentinel address.
- Rejected annotation imports clear the affected table; other tables remain.
  Rejected image loads discard the previous image. A missing file leaves the
  previous contents alone. Errors return to the command prompt.
- Comment saves write the correct end marker, preventing stale table contents
  from entering the saved file. Write errors stop output instead of continuing.

This remains an interactive historical disassembler, not a protected-memory
analysis sandbox. Explicit addresses and offsets can inspect memory beyond the
loaded image. The opcode decoder is retained; the tests do not establish
exhaustive decoding accuracy or that every generated listing reassembles.
The original manual contains some inherited 8080 terminology and examples.

## Tests

```
make test
python3 testpce.py --work-dir /tmp/res86-pce
```

`make test` runs the production CMD through emu2, using real console prompts and
private host files. It checks image loading/rejection, known instructions,
segment boundaries, COM limits, table capacity, interactive insertion and exact
symbol/comment/control save/reload contents across three emu2 OS profiles.
Each prompt has a five-second deadline. Those profiles are not real OS tests.
A separate source-derived diagnostic CMD injects a BDOS read error during each
annotation import and checks table clearing and subsequent recovery.

Validated: 111 production cases and nine diagnostic read-error cases; PCE
smoke tests on CP/M-86 1.1 and CCP/M-86 3.1.

The separate PCE smoke test boots CP/M-86 and CCP/M-86 from private image copies,
checks decoding, segments, invalid-input recovery, and extracts a saved comment
file for byte comparison. It is deliberately outside the Makefile. The named
work directory must not already exist; logs and images are retained there.

See [LICENSE.md](LICENSE.md) for the available licensing evidence.
