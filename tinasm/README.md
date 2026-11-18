# tinasm for CP/M-86

`tinasm` is a small NASM-compatible assembler for the 8086, ported to
CP/M-86 and built with Aztec C (small model) from the cpm86-crossdev
toolchain. It runs on CP/M-86 itself (or under `emu2`), so programs can be
assembled without a cross-compiler.

It produces flat binary output, or a CP/M-86 `.cmd` file for the 8080 memory
model.

## Origin

`tinasm` is based on [Tinyasm](https://github.com/nanochess/tinyasm) by Oscar
Toledo G., with changes from [mininasm](https://github.com/pts/mininasm) by
Peter Szabo. See [`LICENSE.md`](LICENSE.md). The CP/M-86 port keeps only
`tinasm.c`. The original mininasm documentation described a different
program and has been removed.

## Building

Needs the cpm86-crossdev toolchain (`aztec43_acc`, `pcdev_cmdmod`) and
`python3`.

```
make          # builds tinasm.cmd
make test     # assembles the programs in test/ under emu2 and compares them
make clean
```

The Aztec compiler cannot hold the 260-entry instruction table in its string
space, so the table is kept outside the program:

```
instr.txt --mkinstr.py--> instr.bin --pcdev_cmdmod--> group 2 (EXTRA) of tinasm.cmd
```

`tinasm` reads it at run time with the `cmdseg` API. To change the
instruction set, edit [`instr.txt`](instr.txt) and run `make`.

## Usage

```
tinasm -f bin|cmd [options] input.asm -o output
```

| Option           | Meaning                                                      |
|------------------|--------------------------------------------------------------|
| `-f bin`         | Flat binary. The origin is 0 unless the source uses `org`.   |
| `-f cmd`         | CP/M-86 `.cmd` file (8080 model). The source must start with `org 0x100`. |
| `-o file`        | Output file name (required).                                 |
| `-l file`        | Listing file: address, bytes and source of each line, errors, then the label table. |
| `-Dname=value`   | Define a label with a constant value.                        |

Run `tinasm` with no arguments to print a short usage message. Exactly one
input file is accepted.

Example:

```
A>tinasm -f cmd hello.asm -o hello.cmd
A>hello
```

Or under `emu2` on the host:

```
emu2 tinasm.cmd -f bin input.asm -o input.out
```

### CP/M-86 notes

* File names are 8.3. CP/M uppercases the command line, and `tinasm` copes
  with that, so use plain names and no directories, even in `%include`.
* The output file is padded with Ctrl-Z (0x1a) up to a multiple of 128 bytes,
  as CP/M files are made of 128-byte records. Ignore the padding when
  comparing against another assembler's output.
* Aztec C `fopen` is always binary and rejects `"rb"`/`"wb"`.
* The exit code is not seen by `emu2`, because it does not implement BDOS 108.
  Errors are printed on the console and the output file is removed.
* After 128 errors the assembly is aborted.

## Supported source syntax

NASM syntax for the 8086 instruction set (no floating point, no 186/286).

* Labels, with or without a trailing colon before `db`, `dw`, `equ` and
  `times`.
* Numbers: decimal, `0x` hex, `h`-suffix hex (`0ffh`), character constants.
* Expressions with the usual operators and `$` for the current address.
* Data: `db` (numbers and quoted strings, single or double quotes), `dw`.
* Directives: `org`, `align`, `times`, `equ`, `incbin`, `%include`,
  `%if`, `%ifdef`, `%ifndef`, `%else`, `%endif`, `cpu 8086`, `use16`.
* `section` and `segment` are accepted and ignored. The output is flat, so
  write the sections in the order they should appear.
* Comments start with `;`.

### Not supported

These NASM features are not handled:

* `bits 16`, `%define`, `%macro`, `struc`
* `$$` and labels such as `s$$`
* the `jae`, `jna`, `jnae` aliases
* segment overrides inside the brackets, such as `[ss:bp+si+5]`
* forms such as `word msg` as an operand
* a backtick or apostrophe inside a comment

## Tests

`make test` runs `runtest.sh NAME FORMAT REF` for each program in `test/`. It
assembles `test/NAME.asm` under `emu2` and compares the first
size-of-`REF` bytes of the output with the reference image `test/REF`.
It prints `PASS` or `FAIL` per program, and keeps `test/NAME.log` when a test
fails.

The reference images, `test/*.img`, were produced with NASM and, for `-f cmd`,
`bin2cmd` (from cpm86-cmdtools). They use the `.img` extension so that they
are not caught by the `*.bin` and `*.cmd` ignore rules of the parent
repository. They are tracked in git.

`input0`, `input1` and `input2` check the encoding of the instruction set
(`input2.asm` is about 12,000 lines and takes several seconds under `emu2`).
`include` and `hello` use `-f cmd`. The others are real programs: `basic`,
`os`, `bricks`, `fbird`, `invaders`, `pillman` and `rogue`.

## Files

| File                    | Purpose                                              |
|-------------------------|------------------------------------------------------|
| `tinasm.c`              | The assembler (CRLF line endings).                   |
| `instr.txt`             | Instruction table source, `NAME\|operands\|encoding`. |
| `mkinstr.py`            | Compiles `instr.txt` into `instr.bin`.               |
| `cmdseg.h`, `util.lib`  | `cmdseg` API, symlinks to `../util/`.                |
| `Makefile`              | `all`, `test`, `clean`.                              |
| `runtest.sh`            | Runs one test and compares against its reference.    |
| `test/`                 | Sources and reference images.                        |
