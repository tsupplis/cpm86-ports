# SETBOOT test tools

Run from any directory:

    python3 /Users/thierry/Projects/CPM86/cpm86-ports/tools/setboot/test_setboot.py

Build the program first with `make` in `../../setboot`.

The harness executes SETBOOT's actual COM bytes using the existing emu2
8086 CPU core. OS calls, disk reads/writes, reboot, and the INT 2h trap
are intercepted in memory. It never opens a real disk or boots an image.

The emu2 source is read, not modified. The script finds the crossdev source
archive through pcdev_masm1; alternatively set EMU2_SOURCE to src/cpu.c in
an emu2-cpm86 checkout. A C compiler and Python 3 are required.

Generated native executables, synthetic MBRs, and diagnostic captures live
in work/. The register-clobber checks exercise saves needed by the output
callers, including ES preservation on BDOS calls. Prefix and initial-DS
failures must trap before any OS or disk call.

These checks supplement collaborative tests on the actual operating
systems; they do not establish hardware/XIOS behavior.
