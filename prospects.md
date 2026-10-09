# CP/M-86 porting candidates — web sweep summary

Confidence legend:
- ✅ **Confirmed** — a live source listing/download was actually fetched and checked in this session.
- ⚠️ **Reasoned** — well-attested in CP/M history/retrocomputing sources, but no source archive was directly opened/verified this session; needs a follow-up check before committing to port.
- ❌ **Rejected** — considered and ruled out, reason given.

## 1. Communications / file transfer (confirmed hard gap)

Neither your list nor cpm86-hacking has a terminal emulator or file-transfer protocol
— only `conio` demos and keyboard scanners.

| Tool | What it is | Author / origin | License | Source pointer | Status |
|---|---|---|---|---|---|
| **Kermit-86 (IBM PC/XT target)** | Kermit protocol client/terminal | Columbia University Kermit Project | Free, source included, Kermit license (permissive, attribution) | [columbia.edu/kermit/cpm.html](http://www.columbia.edu/kermit/cpm.html) → links to [kermit software archive, CP/M section](http://www.columbia.edu/kermit/fixed/archive.html#cpm80) (tar/zip of all files) and [c86ker.pdf](https://www.columbia.edu/kermit/ftp/cpm86/c86ker.pdf) user guide | ✅ Page confirms "source code is included for all versions" (CP/M-80/85/86/68K/Concurrent CP/M); [retrocmp.de](https://www.retrocmp.de/transfer/modem9.htm) separately confirms no PC/XT-specific CP/M-86 Kermit build exists yet — genuine gap to fill from the generic CP/M-86 source |
| **MEX** | Terminal program w/ XMODEM, built on Ward Christensen's MODEM standard | Ron Fowler | Public domain / freely distributed (era-typical) | [zimmers.net/.../comm/mex/](http://www.zimmers.net/anonftp/pub/cpm/comm/mex/index.html) | ⚠️ Directory confirmed live, but only `.com`/`.lbr`/`.hlp`/overlay files were visible in the listing fetched — need to open the `.lbr` archives (e.g. `mex-c128.lbr`) to confirm `.asm` source is actually inside before committing |
| **BYE** | Remote-access host/terminal program | — | — | [zimmers.net/.../comm/bye/](http://www.zimmers.net/anonftp/pub/cpm/comm/bye/index.html) | ⚠️ Directory confirmed, contents are `.lbr`/`.iqs` (squeezed) binaries — source presence unconfirmed |
| **IMP** | RS-232 terminal program | — | — | [zimmers.net/.../comm/imp/](http://www.zimmers.net/anonftp/pub/cpm/comm/imp/index.html) | ⚠️ Directory confirmed, only `.com` binaries seen in this pass |
| **qterm** | VT100 terminal, Kermit + XMODEM protocols | — | — | [zimmers.net/.../comm/qterm.lzh](http://www.zimmers.net/anonftp/pub/cpm/comm/qterm.lzh) | ⚠️ Confirmed file exists; LZH contents (source or binary-only) not opened |

## 5. Spreadsheet (same empty-PD-culture situation as database)

| Tool | What it is | License | Source pointer | Status |
|---|---|---|---|---|
| **sc (Spreadsheet Calculator)** | Full cell/formula spreadsheet, vi-like keybindings | Public-domain origin (James Gosling / Mark Weiser / Robert Bond, 1981) | [github.com/n-t-roff/sc](https://github.com/n-t-roff/sc) (fork of upstream 7.16, Sept 2002, originally mirrored at ibiblio.org) | ✅ Confirmed live repo. Caveat: screen I/O is curses-based — would need retargeting to cpm86-hacking's existing `conio.c`/`conio.h`, not a straight port |
| VisiCalc | The original spreadsheet | Historical/research-only (Bricklin) | [github.com/hwatkins/visicalc](https://github.com/hwatkins/visicalc) (annotated disassembly) | ❌ Rejected — 6502 Apple II assembly (wrong CPU family vs. 8080/8086), and license terms are research-only, not redistribution-friendly |
