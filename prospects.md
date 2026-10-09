# CP/M-86 porting candidates — web sweep summary

Context: existing CP/M-86 tool collection (first table, user-supplied) + the
[cpm86-hacking](https://github.com/tsupplis/cpm86-hacking) sister project (unix-like
tools, submit tools, general CLI tools). Constraints: ASM80 (8080, not Z80) source
translatable to 8086, or pre-ANSI/K&R C portable sources. vi is already covered
(cpm86-vi / STevie port). Preference for permissive licenses (public domain / BSD /
MIT / freeware), matching the style of the existing list.

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
| **uuencode/uudecode for CP/M** | Binary-to-text transport encoding | — | — (era freeware) | [zimmers.net/.../comm/uucode.lbr](http://www.zimmers.net/anonftp/pub/cpm/comm/uucode.lbr) | ✅ Listed and described as "uuencode and uudecode for CP/M"; `.lbr` extraction needed to confirm source is bundled (LU/NULU from your own list already extracts `.lbr`) |
| **BYE** | Remote-access host/terminal program | — | — | [zimmers.net/.../comm/bye/](http://www.zimmers.net/anonftp/pub/cpm/comm/bye/index.html) | ⚠️ Directory confirmed, contents are `.lbr`/`.iqs` (squeezed) binaries — source presence unconfirmed |
| **IMP** | RS-232 terminal program | — | — | [zimmers.net/.../comm/imp/](http://www.zimmers.net/anonftp/pub/cpm/comm/imp/index.html) | ⚠️ Directory confirmed, only `.com` binaries seen in this pass |
| **qterm** | VT100 terminal, Kermit + XMODEM protocols | — | — | [zimmers.net/.../comm/qterm.lzh](http://www.zimmers.net/anonftp/pub/cpm/comm/qterm.lzh) | ⚠️ Confirmed file exists; LZH contents (source or binary-only) not opened |

## 2. Archiving (improves on your xsq/lbr squeeze-only coverage)

| Tool | What it is | Author / origin | License | Source pointer | Status |
|---|---|---|---|---|---|
| **LHarc / LZH** | Multi-file compressed archive (vs. squeeze's single-file-only) | Haruyasu Yoshizaki | Freeware, source available | Not pinned to a live URL this session | ⚠️ Reasoned — well-documented freeware with historical CP/M ports, but no archive was directly opened/verified |
| **ARC (SEA)** | The iconic DOS/CP/M archiver | Thom Henderson / System Enhancement Associates | Disputed — SEA vs. PKWARE lawsuit history | [Wikipedia: ARC (file format)](https://en.wikipedia.org/wiki/ARC_(file_format)), [BBS Documentary SEA library](http://www.bbsdocumentary.com/library/CONTROVERSY/LAWSUITS/SEA/) | ❌ Rejected — source history is legally contested, not a clean freeware grant like the rest of your table |

## 3. Unix-culture text tools

Ported from the same RetroBSD/BSD source tree as `grep`.

| Tool | What it is | Source pointer | Status |
|---|---|---|---|
| **grep** | Search a file for a pattern | RetroBSD / DiscoBSD BSD tree | ✅ Ported — `grep/` |
| **sed** | Stream editor | Same BSD tree | ✅ Ported — `sed/` |
| **diff** | File comparison | Same BSD tree | ✅ Ported — `diff/` (file-to-file only; directory diff dropped) |
| **awk** | Pattern-scanning/text processing | Same BSD tree | ❌ Near showstopper — requires lex and yacc as prerequisites; impractical on CP/M-86 |
| RATFOR for CP/M (tangential) | Ratfor compiler running on CP/M via z88dk | [github.com/jayacotton/RATFR](https://github.com/jayacotton/RATFR) | ✅ Confirmed live repo — Ratfor *language* toolchain only, not a ready text tool |

## 5. Spreadsheet (same empty-PD-culture situation as database)

| Tool | What it is | License | Source pointer | Status |
|---|---|---|---|---|
| **sc (Spreadsheet Calculator)** | Full cell/formula spreadsheet, vi-like keybindings | Public-domain origin (James Gosling / Mark Weiser / Robert Bond, 1981) | [github.com/n-t-roff/sc](https://github.com/n-t-roff/sc) (fork of upstream 7.16, Sept 2002, originally mirrored at ibiblio.org) | ✅ Confirmed live repo. Caveat: screen I/O is curses-based — would need retargeting to cpm86-hacking's existing `conio.c`/`conio.h`, not a straight port |
| VisiCalc | The original spreadsheet | Historical/research-only (Bricklin) | [github.com/hwatkins/visicalc](https://github.com/hwatkins/visicalc) (annotated disassembly) | ❌ Rejected — 6502 Apple II assembly (wrong CPU family vs. 8080/8086), and license terms are research-only, not redistribution-friendly |

## Open follow-ups before committing to any port

1. Open `mex-c128.lbr` (or an equivalent MEX distribution) to confirm `.asm` source is bundled, not just overlays/binaries.
2. Pull the actual CP/M-86 Kermit file set from the Kermit software archive link above and confirm the PC/XT-specific overlay is buildable from the generic CP/M-86 source + your own BIOS calls.
3. Extract `uucode.lbr` to confirm source presence.
4. Locate a live, verifiable LZH/LHarc CP/M source archive (not just historical attestation).
5. Confirm whether your BSD Unix-tools tree actually includes `ndbm`/Berkeley DB 1.85 before treating it as "in hand."
