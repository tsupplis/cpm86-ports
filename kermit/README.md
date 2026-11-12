# CP/M-86 Kermit — IBM PC/XT port

CP/M-86 Kermit-86 v2.9, ported to the IBM PC/XT.

**Authors:** Bill Catchings, Ron Blanford, Richard Garland (Columbia / U. Washington)  
**Language:** Digital Research ASM86  
**Version:** 2.9 (December 1984)  
**Documentation:** Frank da Cruz, Columbia

## Building

```
make
```

Produces `kermibm.cmd` (CP/M-86) via `cpm86_asm86` + `cpm86_gencmd`.

## Using Kermit on CP/M-86 and CCP/M-86

The same `KERMIBM.CMD` and commands work on both systems. Select the drive
containing your files, then start Kermit:

```text
A>B:
B>KERMIBM
Kermit-86 B>SET BAUD 2400
Kermit-86 B>SET FILE-TYPE BINARY
```

Use COM1 on both machines (the default), with matching baud rates and 8N1.
For physical PCs, connect their serial ports with a null-modem cable.

| Receiving machine — start first | Sending machine |
|---|---|
| `RECEIVE` | `SEND MYFILE.CMD` |

Files arrive on the receiver's current drive. Wait for **both Kermit prompts**
before starting another transfer or reversing direction. Use `BINARY` for
programs and data; use `SET FILE-TYPE ASCII` on both sides for CP/M text files
(CR/LF lines, Ctrl-Z end-of-file).

`Ctrl-C` aborts a transfer; `EXIT` returns to the operating system. Type `?`
for help. `CONNECT` opens a terminal session over the serial link; press
`Ctrl-\` followed by `C` to return to Kermit. Optional setup commands can go in
`KERMIT.INI` on the current drive; Kermit reads it automatically at startup.

## Testing with two PCE machines

```mermaid
flowchart LR
    A["PCE 1: CP/M-86<br/>Kermit · COM1"] <-->|"Raw TCP · virtual null modem"| B["PCE 2: CCP/M-86<br/>Kermit · COM1"]
    DA["Private boot + data disks"] --- A
    B --- DB["Private boot + data disks"]
```

Run this external integration test directly from this directory (`make test`
is empty; it does not launch PCE):

```sh
make
python3 testpce.py --work-dir /tmp/kermit-test
```

The harness starts a TCP listener in the first PCE and connects the second to
it on localhost. Both emulate an 8250 at COM1 (`03F8h`, IRQ4); `telnet=0` keeps
the link binary-safe and `usectl=0` disables PCE's modem-control protocol.
The TCP port is selected automatically. Each machine gets its own disk copies;
the original images are untouched.

The test sends a 1 KiB binary containing every byte value and a text file
**in both directions**, compares the received contents, and checks the transfer
display, clean exit, interrupt-vector restoration and restart on both systems.
It starts each receiver first and waits for both prompts between transfers.

Requires Python 3 (no extra packages), `pce-ibmpc`, `cpmcp`, and the repository's
OS images, base data image, configurations and ROMs. The output directory must
be new; it retains disk images, serial logs and screen dumps. Omit `--work-dir`
for temporary output. Add `--baud 9600` to test at 9600 instead of 2400 baud.

Validated at **2400 baud on CP/M-86 and CCP/M-86** with all checks passing.
Tests use a standard-speed 8088 and timers off, with a host-side deadline.
Physical hardware, COM2 and other Kermit implementations are outside this test.

## Source modules

| File | Description |
|------|-------------|
| `c86ker.a86` | Top-level routines, messages, prompts |
| `c86cmd.a86` | Command parser |
| `c86fil.a86` | Send/receive file handlers |
| `c86pro.a86` | Protocol state machine |
| `c86trm.a86` | Terminal emulation |
| `c86utl.a86` | CP/M-86 utility routines |
| `c86xibm.a86` | **IBM PC/XT system-dependent I/O** (this port) |

`c86ker.a86` drives the build via `INCLUDE` directives; the machine-dependent
module is included as `C86XIBM.A86`.

## Machine-dependent interface (`c86xibm.a86`)

### Port I/O
| Routine | Description |
|---------|-------------|
| `prtout` | Send character in AL to serial port (parity set by `dopar`) |
| `instat` | Skip-return if character available in ring buffer |
| `inchr`  | Return next character from ring buffer in AL |
| `cfibf`  | Clear the receive ring buffer |
| `prtbrk` | Send a 250 ms BREAK |
| `serini` | Initialise 8250, hook interrupt vector, set baud/port |
| `serfin` | Restore interrupt vector, shut down 8250 |
| `bdset`  | `SET BAUD` command handler (300/1200/2400/4800/9600) |
| `prtset` | `SET PORT` command handler (COM1/COM2) |
| `shobd`  | Display current baud rate |
| `shoprt` | Display current port |

### Screen control (VT52)
`poscur`, `clrscr`, `clrlin`, `clreol`, `revon`, `revoff`, `bldon`, `bldoff`, `dotab`

VT52 sequences: cursor position (`ESC Y row col`), clear screen (`ESC H ESC J`), erase to EOL (`ESC K`). Reverse video and bold are no-ops (VT52 has no video attributes).

### Hardware
- **UART:** 8250 on COM1 (`03F8h`) or COM2 (`02F8h`)
- **IRQ:** COM1 = IRQ4 (int `0Ch`), COM2 = IRQ3 (int `0Bh`)
- **Receive:** interrupt-driven ring buffer (256 bytes)
- **Transmit:** polled via LSR THRE bit
- **Default:** COM1, 2400 baud, 8N1

## Capabilities

| Feature | Support |
|---------|---------|
| Text file transfer | Yes |
| Binary file transfer | Yes |
| Wildcard send (alphabetical) | Yes |
| ^X/^Z interruption | Yes |
| Filename collision avoidance | Yes |
| 8th-bit prefixing | Yes |
| Terminal emulation (ANSI/VT100) | Yes |
| Baud rate setting | Yes (300–9600) |
| Port selection | Yes (COM1/COM2) |
| XON/XOFF flow control | No (RTS/DTR asserted; CTS is not polled) |
| Talk to Kermit server | Yes (SEND/GET/FIN/BYE) |
| Act as server | No |
| Transaction logging | No |
| Session logging (raw download) | Yes |

## Commands

`CONNECT`, `SEND`, `RECEIVE`, `GET`, `BYE`, `LOGOUT`, `FINISH`, `EXIT`/`QUIT`,
`SET`, `SHOW`, `TAKE`, `LOCAL` (SPACE / DIRECTORY / DELETE / TYPE)

Key `SET` parameters: `BAUD`, `PORT`, `PARITY`, `ESCAPE`, `FILE-TYPE`,
`FLOW-CONTROL`, `IBM`, `LOCAL-ECHO`, `LOG`, `TIMER`, `WARNING`, `DEFAULT-DISK`

## v2.9 changes (December 1984)

- `LOCAL DIRECTORY` now computes file sizes correctly for all files
- `LOCAL TYPE` implemented — wildcard filespec, alphabetical, paged with `--more--`
- Wildcard `SEND` sends files in alphabetical order; accepts optional start filename
- `C86PRO.A86` made resilient to echoed-back packets (Chris Lock, Nottingham)
