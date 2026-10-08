seemem
======

`seemem` — a full-screen CP/M-86 memory browser. It shows memory as
`segment:offset`, 16 bytes per line in hex and ASCII, 24 lines per screen,
starting at 0000:0000.

SEEMEM 0.1, freeware from Frank Kotler and Kirk Lawrence, written for CP/M-86
on the IBM PC.

## Synopsis

```
seemem
```

There are no command line arguments.

## Keys

`seemem` reads scan codes straight from the BIOS keyboard service.

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
  memory at B800h (colour) or B000h (monochrome), and uses BIOS interrupts
  11h and 16h.
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
