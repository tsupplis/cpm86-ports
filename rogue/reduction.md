# Rogue: code size reduction

## Problem

`rogue.cmd` does not link. The small model has a 64K code segment, and the
linker stops at the first library module that overflows it:

```
file: D:\lib\c86.lib(write) Code section > 64k.
Code: 077d  Data: 3ee2  Udata: 284d
```

`Code: 077d` is the 16-bit wrap of 0x1077d, about 67.4 KB. The real total is
not known, because the linker stops at the first overflow and the remaining
library modules are not counted. Data (16 KB) and Udata (10 KB) are fine.

The game code needs to shrink by at least about 5 KB, probably more.

## Current size

The objects add up to about 54.8 KB of code. The libraries (`c86.lib`,
`util.lib`) and the startup code add at least 12.6 KB on top.

| File | Bytes |
| --- | --- |
| level | 5898 |
| monster | 4859 |
| invent | 3828 |
| room | 3750 |
| use | 3385 |
| move | 3263 |
| object | 3099 |
| spec_hit | 2968 |
| save | 2709 |
| pack | 2669 |
| zap | 2342 |
| score | 2116 |
| hit | 1999 |
| throw | 1901 |
| message | 1673 |
| curses | 1561 |
| trap | 1513 |
| init | 1489 |
| ring | 1278 |
| play | 912 |
| fmt | 632 |
| random | 445 |
| machdep | 442 |
| main | 80 |

To refresh the numbers, take the last `Code (xxxx)` line of `aztec43_obd file.o`.

## Done so far

- `-Osize` is first in `CFLAGS` (about 6% smaller).
- Float and `snprintf` use removed: `mon_hit` and `potion_heal` use integer maths.
- Debug and wizard code removed: `show_average_hp`, `wizardize`,
  `c_object_for_wizard`, `show_traps`, wizard-only keys in `play.c`.
- All `if (wizard)` branches and the `wizard` variable removed. It was always 0.
- Unused `lget_number` and `r_index` removed.
- Score file and top ten replaced by a stub in `score.c`.
- `fmt.c`: small integer-only `sprintf` and `printf` (`%d %ld %s %c %%`, `0`
  flag and width). They replace the `c86.lib` versions, which pull in the whole
  float-capable formatter.
- `doprnt.c` is not built.

None of these has been shown to move the link far enough. The failing module
changed (`snprint`, `ftoa`, `csav`, `write`, `malloc`, `write`), but there is
no way to see which library modules get pulled in.

## Not worth doing

- Deleting the termcap block in `curses.c`: `-DANSI` already compiles it out.
- Looking for more dead functions: a check for functions with no callers finds
  none left.

## Next steps

1. **Measure.** Get the real total instead of guessing. Link the same objects
   for a target without the 64K limit (the large model, DOS `.exe`; see
   `docs/aztec43/acc.md`, "Non-small model"), or link with `-Wl,-M -Wl,-V`
   and read the segment table. This gives the exact size to remove.
2. **Cut features.** Candidates, with approximate code size:
   - `save.c` save and restore (2.7 KB). This removes the save game.
   - `room.c` options menu (`edit_opts`) and `do_shell`.
   - `spec_hit.c` rarely used monster abilities (3 KB).
   - Help screens and command descriptions in `invent.c`.
   - Other `sprintf` messages that could be plain strings.

   Ask before dropping anything that changes game play.
3. **Overlays** if cutting is not enough. The toolchain supports them:
   - Link the root with `-overlay-root=CODE,DATA`. This writes a `.rsm` file.
   - Link each overlay with `-overlay=root.rsm` into a `.ovr` file.
   - The root calls `ovloader("name", a, b)`. The overlay entry is
     `ovmain(int a, int b)`.
   - Example: `cpm86-crossdev/examples/` (`Makefile`, `ovrmain.c`, `ovrfunc1.c`).
   - Candidates: `save.c`, `score.c`, `init.c`, and the rarely used scroll,
     potion and zap code.

   The game stays whole, but the build gets more complicated.
4. **After it links.** Run under `emu2` with `timeout -f`. Check the ANSI
   output, `getch` with arrow and escape keys, and the 24x80 layout. Add a
   test or README, and a row in the top `README.md` (Games table).
