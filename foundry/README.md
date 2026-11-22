# Foundry

A falling-block game for the CP/M-86 text console. Fit seven shapes made
of three, four, or five tiles into an 8×14 well. A full horizontal row
clears regardless of tile color, and the rows above it move down together.
Clearing 1, 2, or 3 rows at once scores 100, 400, or 900 points.

The dashboard fits an 80×24 screen. The well uses CP437 borders and updates
only changed cells. The side panel shows the next piece, score, line count
and controls. The color display uses the CP/M-86 VT52 extension and two
solid CP437 blocks (DBh) per tile. Monochrome uses distinct patterns to
help distinguish adjacent pieces.

| Key | Action |
| --- | --- |
| Left / Right or A / D | Slide left / right |
| Up or W | Rotate clockwise |
| Down or S | Step down |
| Space | Drop and settle |
| Q / Esc | Quit |

Build with `make` (Aztec C 4.3) and run `FOUNDRY`. The build uses
`conio.h` and `util.lib` from the sibling `cpm86-hacking` checkout. Set
`HACKING` in the Make invocation to use another checkout. Use `FOUNDRY -M` for
monochrome, or `FOUNDRY -S` for manual stepping. In manual mode the console
may require Enter after each command. Use Q or Esc followed by Enter to quit
in manual mode. The default timed mode uses BIOS
ticks on IBM PC CP/M-86 1.1 and BDOS delay on CCP/M-86 3.1 or Concurrent DOS
4.1.

The game rules live in `foundry.c`. The console and pacing live in
`fdplay.c`, with an assembly entry point for the IBM PC timer.
`make test` checks the rules on the host and exercises the native CMD
in manual mode under three emu2 CP/M profiles, in color and monochrome.
The scripted VT52 arrow inputs are checked against the resulting tile positions.
Arrow keys require the console to deliver a VT52 or ANSI escape sequence.
Use A/D/W/S if the console consumes the arrow keys.
Timed falling, video attributes and return to the shell were also checked
in PCE on CP/M-86 1.1, CCP/M-86 3.1 and Concurrent DOS 4.1.
