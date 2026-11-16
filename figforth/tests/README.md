# fig-FORTH regression tests

From the parent directory, run:

```sh
make test
```

To select an emulator:

```sh
make test EMU2=/absolute/path/to/emu2
```

`test.py` runs the native `forth.cmd` with emu2's 8086 CPU model and both
2.2 and 3.1 BDOS profiles. Every suite uses a temporary directory and its
own `FORTH.BLK`; existing user block files are not modified.

Coverage includes arithmetic, stacks, compiled words, loops, conditionals,
variables, `DOES>`, `FORGET`, unknown-word recovery, dirty-memory startup,
blank blocks and file gaps, persistence across processes, extent boundaries,
`BYE` flushing, source-screen loading, and out-of-range blocks. The five
shipped examples are loaded from `examples/demo.blk` and their output is
checked too. Each emulator run has a timeout.

PCE remains a separate manual check. Boot CP/M-86 with a private data disk,
run the `SQUARE` and save/restart examples in the parent README, and run the
five example screens. Emulator BDOS profiles do not replace real-OS tests.
