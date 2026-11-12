# Sokoban

CP/M-86 port of a Unix curses clone of sokoban.

**Freeware by Russell Marks (ber@astbe).**

I found it as part of the ZCN repository: https://github.com/jamesots/zcn

The C source has been modified for VT52 terminal compatibility, and some
unused variables have been removed.

`sokoban.cmd` is the compiled CP/M-86 binary.

Only `sokoban.cmd` is needed on the CP/M-86 disk. Levels and help are
embedded in the executable. Press `?` for paginated help; any key advances,
and `Q` or Escape returns early to the unchanged game.

## Commands

| Key   | Action              | Key    | Action         | Key | Action              |
| ----- | ------------------- | ------ | -------------- | --- | ------------------- |
| h     | move/push left      | H      | run/push left  | ^H  | run left to object  |
| l     | move/push right     | L      | run/push right | ^L  | run right to object |
| j     | move/push down      | J      | run/push down  | ^J  | run down to object  |
| k     | move/push up        | K      | run/push up    | ^K  | run up to object    |
| u     | undo last move/push | U      | undo all       | 5   | undo last move/push            |
| c / s | temporary save      | ^U / r | reset to temporary save | | |

| ^R | Refresh screen | q | quit |

`?` displays help. Temporary saves (`c` / `s`) are in memory only, not on disk.

The arrow keys also move the player. VT52 (`ESC A`..`ESC D`) and VT100
(`ESC [ A`..`ESC [ D`) sequences are both accepted, as are the numeric keypad
(`8` `2` `4` `6`) and `^E` `^X` `^S` `^D`.

## The game

Characters in the level files, and how they are drawn on the IBM PC:

| Symbol | On screen | Colour | Meaning |
| ------ | --------- | ------ | ------- |
| `@` | `ΘΘ` | bright red | player |
| `+` | `ΘΘ` | bright red | player on saving position |
| `.` | `∙∙` | bright cyan | saving position for packet |
| `$` | `■■` | bright yellow | packet |
| `*` | `■■` | bright green | saved packet |
| `#` | `██` | green | wall |

The board is drawn two lines below the top of the screen.

Your goal is to move all packets to the saving position by pushing them.

As you could see you can make a temporary save. This is useful if you think
that all the moves/pushes you have made are correct, but you don't know how
to go on. In this case you can temporary save (using the c command). If you
then get stuck you need not undo all (using U), you can reset to your
temporary save.

Entering a level automatically creates its initial temporary checkpoint.

## Compiling

Built with the Aztec C compiler (`aztec43_acc`) targeting CP/M-86:

```sh
make
```

This produces `sokoban.cmd` linked against the CP/M-86 C library (`-lc86`).
Terminal I/O uses the shared `conio.h` and `util.lib` from `cpm86-hacking`
that provides `getch()`, `clrscr()`, `gotoxy()` and `textcolor()`.

## Concurrent CP/M keyboard

On concurrent BDOS >= 3.0, the game selects raw function keys with `ESC 6`
and maps the XIOS arrow codes to its existing movement commands. Exit restores
normal expansion with `ESC 7`. CP/M-86 and terminal escape handling are
unchanged. Buffered console bytes are returned unsigned by the shared library.

`make test-keys` runs the game adapter on the host with mocked shared-console
calls, checking high-bit keys, coordinates and mode restoration. Verify
physical arrow movement and normal function keys after quitting on the target.

The build uses local copies of `util.lib`, `conio.h` and `util.h` supplied from
`cpm86-hacking`. It does not build or require the sibling project. There is no
local conio implementation. `gameio.c` contains game-specific key translation
and a coordinate adapter: Sokoban uses column,row while the shared `gotoxy`
takes row,column.

## Embedded resources and tests

`cmdmod` installs `soklevls.dat` and `sokoban.hlp` in one EXTRA group. These
remain editable build inputs; changing either rebuilds the CMD. The header
is `SKR1` followed by two pairs of little-endian 16-bit offset/length values
(levels, then help). Reads stop at those lengths, not CMD padding. Minimum
and maximum group allocation match the packed size rounded to a paragraph.
Help uses `+=` and `==` lines for page break and end, with automatic paging
if a page grows beyond 22 lines.

`resource.c` retains a file-backed alternative for DOS/non-CP/M builds,
using the original filenames. The game still builds CMD only; this preserves
the DOS resource path without introducing a complete DOS console port.

`make test` includes the keyboard tests and Aztec/emu2 regressions for all
50 levels, help navigation, board/undo/checkpoint preservation, resource
bounds, invalid headers, and rebuilding after either resource changes.
The file-backed reader is also compiled and executed as a DOS 1 COM program.
Builds/tests use Python's standard library and the existing compiler,
emulator, cmdmod and host C compiler.
