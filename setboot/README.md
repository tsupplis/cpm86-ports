# HELLO: minimal DOS / CP/M-86 launch detector

Build with `make hello`. This uses MASM 1.10 and the existing crossdev
commands. Run `hello.com` on DOS or `hello.cmd` on CP/M-86. The two files
contain identical program bytes; the CMD has an 8080-model header and an
entry point of 0100h.

Output is one line:

    Hello world - DOS

or:

    Hello world - CP/M-86

`detect_os` returns AL=0 for DOS or AL=1 for CP/M-86. An unknown layout
traps through INT 2h. It checks the original DS before changing it. HELLO
installs a private 512-byte stack before calling the detector.

The assumptions are a DOS COM load or a CP/M-86 8080-model CMD load, on an
IBM PC compatible. Checks are:

1. Entry DS equals CS.
2. The prefix matches: DOS has CD 20 at offset 0; a CP/M 8080 base page
   has 01 at offset 5 and its code-group segment equals CS at offset 3.
3. On the recognized CP/M side only, the INT E0h vector must be nonnull,
   then BDOS function 12 is called. Every BDOS call preserves ES.

ES is not required to equal CS at entry, and SS may differ from CS. The
BDOS wrapper saves only ES: its callers do not need other registers saved.
The detector promises AL as its result and preservation of ES; main sets
DS back to CS afterwards. DOS calls are inline, without a save-all wrapper.

These are structural checks and a call in the recognized BDOS context.
A nonnull vector alone is not proof that an API exists, and a version call
has no universal success signature. No exact returned version is required.
DOS CALL-5 details are not required to match one implementation.

The result describes the launch interface. It does not identify DOS Plus,
Concurrent DOS, or another host that supplies compatibility interfaces.
There is no probe of INT E0h on the DOS side, and no disk access or reboot.

An unknown layout invokes the NMI handler using the software instruction
INT 2h. If the handler returns, the program traps again. There is no error
message, guessed OS exit, or loader-stack recovery.

MASM 1.10 builds without errors or assembler warnings. The linker warning
about no EXE stack segment is expected: this COM sets its own SS:SP.
Both files have run on emu2's 8086 CPU, including the CMD with memory
initially filled with FFh and compatibility version 2.2. Payload equality
has been checked. Actual OS and hybrid-system testing remains collaborative.
