
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
     <-  - File Left                        O - shOw System Files
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
