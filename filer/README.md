
VFILER Version 1.2 - CP/M-86 File Manipulation Program
  Original ZCPR2 version by Rich Conn
    CP/M-86 translation by H. M. Van Tassell
      Optimized for IBM-compatibles by Kirk Lawrence


Note: This version of VFILER is specific to IBM-compatible computers.
It re-programs the keypad keys to make them compatible with VFILER's
existing code.

To move the on-screen pointer (--): use the ARROW keys (up, down, left, right)

To move the screen up or down a page:  use the PGUP and PGDN keys.


-- Tagging Commands --                  -- File Operations --
    T - Tag File                            C - Copy File
    U - Untag File                          D - Delete File
    W - Mass Tag/Untag                      F - File Size
                                            M - Mass Copy
-- File Print & View --                     R - Rename File
    P - Print                               Y - Mass Delete
    V - View

-- Movement Commands --                  -- Miscellaneous --
     <-  - File Left                        O - Toggle normal/all (SYS/HIDDEN)
     ->  - File Right                       E - Enter Command
        - File Up                          L - Login DIR
        - File DOwn                        N - reNew Screen
      G  - Go To a File                     S - Status of Disk
   PGDN  - Screen Forward            ESC or Q - Quit VFILER
   PGUP  - Screen Backward              ENTER - execute a CMD file


Concurrent CP/M keyboard support
------------------------------

Concurrent BDOS >= 3.0 uses ESC 6 for raw function-key input. FILER translates
high-bit keypad codes to the scan-code pairs expected by its command table.
CP/M-86 retains VT52 input. ESC 7 restores normal expansion on exit and before
launching a command or HELP; returning actions select raw input again.

`make test-keys` assembles and executes the real keyboard routines in emu2,
with mocked OS input. It checks arrows, keypad, VT52 compatibility, version
selection and expansion control. On the target verify navigation, Quit and
external command handoff, including an unsuccessful chain.

Concurrent command execution
----------------------------

On a concurrent system, E, X and external HELP use native P_CLI and leave FILER
resident. FILER waits to reclaim its console, restores user, drive, priority
and DMA, and redraws the current directory and selection. Its recovery priority
runs ahead of the parent and TMP. Launch failure leaves FILER active with an
error message. Native program exit status is not interpreted.

CP/M-86 retains P_CHAIN. The help screen keeps the [*] warning there; concurrent
help shows "eXecute CMD file" and states that E/X return after the action.

`make test-exec` runs the actual dispatch assembler with mocked BDOS calls in
emu2, checking P_CLI success/failure, nested priorities, console handoff,
context recovery, repeated returns without stack growth and the legacy chain.
Verify E/X and external HELP on the booted concurrent system.

SYSTEM and FAT HIDDEN files
---------------------------

Normal-only is now the default on CP/M, CCP/M and CDOS. O toggles all files,
including SYSTEM and FAT HIDDEN. The selected-file status line distinguishes
S and H; the four-column grid and tagging keys retain their existing layout.
An empty filtered list offers the O toggle rather than sorting an empty ring.

A separate byte accompanies each name through sorting, paging, tagging and
deletion. HIDDEN never becomes a CP/M filename attribute. FAT allocation-byte
interpretation requires both a version gate and a successful raw FAT header
probe. No BDOS 230 or DOS interrupt is called; CP/M allocation bytes retain
their original meaning.

FILER retains its historical READ-ONLY override for confirmed deletion and
replacement, but now clears only READ-ONLY. BDOS 17 retrieves existing name
attribute bits into private DMA; BDOS 30 preserves SYSTEM/archive/F1-F3.
Missing search results restore DMA without changing attributes. HIDDEN is
retained by the OS attribute backend. Locks and disk write protection still
apply. Deleting FAT HIDDEN files requires the updated CDOS BDOS 19 backend.

make test-keys test-exec test-attrs checks the real assembler routines with
mocked OS input. The attribute check exercises all four search-buffer slots,
RO/SYSTEM/archive combinations, F1-F3 preservation and missing-file cleanup.
Live tests must pace command keys after a screen redraw: FILER's existing
output flow-control poll consumes keys received during drawing.

The independent live test (from the ports root) is:

```
python3 tools/hidden_browser_check.py filer --boot BOOT.img --config PCE.cfg
```

Add --classic --blank BLANK320.img for an original CP/M/CCP/M boot disk:
it tests a SYSTEM+READ-ONLY CP/M file, with no FAT metadata dependency.
Boot disks and data templates are copied; supplied originals are untouched.

Verification passed on CDOS FAT, original CP/M-86 1.1 and CCP/M-86 3.1.
