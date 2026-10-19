seemem
======

`seemem` — a full-screen CP/M-86 memory browser. It shows memory as
`segment:offset`, 16 bytes per line in hex and ASCII, 24 lines per screen,
starting at 0000:0000.

SEEMEM 0.2, freeware from Frank Kotler and Kirk Lawrence, written for CP/M-86
on the IBM PC.

## Synopsis

```
seemem
```

There are no command line arguments.

## Keys

On CP/M-86, `seemem` reads scan codes from the BIOS keyboard service.
On concurrent BDOS >= 3.0, it uses BDOS input and raw XIOS function-key codes,
including in the GOTO prompt. It never calls INT 16h on that path. ESC 6 selects
raw keys and ESC 7 restores normal expansion on exit.

| Key | Action |
| --- | --- |
| `PgDn` | Next screen |
| `PgUp` | Previous screen |
| `Down` / `Up` | Scroll one line |
| `Home` / `End` | Start / end of the current 64K segment |
| `+` / `-` | Start of the next / previous 64K segment |
| `G` | Go to a `segment:offset` address, typed in hex (`Esc` cancels) |
| `Esc` | Quit |

## Requirements

- An IBM PC compatible with CP/M-86. The program writes directly to video
  memory at B800h (colour) or B000h (monochrome). On concurrent systems it
  reads the equipment word from the BIOS data area and clears the status row
  directly: INT 10h, 11h and 16h are used only on non-concurrent systems.
- It turns off the CP/M-86 status line while it runs.

## Building

```sh
make          # seemem.cmd
make clean
```

See https://github.com/tsupplis/cpm86-crossdev for the cross-development
environment.

## License

Freeware by Frank Kotler and Kirk Lawrence. No license text comes with the
source. It is treated here as public domain.

## Keyboard verification

`make test-keys` executes the actual keyboard assembler in emu2 with mocked
BDOS/BIOS input. It checks arrows, paging, Home/End, GOTO characters and cancel,
plus/minus, legacy BIOS selection and preservation of the browsed segment and
working registers. On the target verify scrolling, GOTO, Backspace/Esc and
normal function keys after exit. Direct video memory output remains IBM-PC
specific; this change does not virtualize the display.

`make test-startup` exercises the full browser startup, first screen and exit
with mocked OS calls, on legacy/concurrent and mono/colour paths. Concurrent
BIOS calls fail the test, catching the XIOS trap that previously caused a beep
and termination before the first screen.
