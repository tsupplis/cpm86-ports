KERMIT FOR CP/M-86

The version of Kermit implemented under the CP/M-86 operating system
is called Kermit-86 (as is the MSDOS/PCDOS version).  The source file
names all start with the prefix "c86".  

------------------------------

There are six machine-independent source modules in Kermit-86, and
one machine-dependent module which is implemented separately for each
machine supported.

The machine-independent modules are:
	c86ker.a86 - top-level routines and system messages & prompts
	c86ker.a86 - command parser
	c86fil.a86 - send and receive local file handlers
	c86pro.a86 - send and receive state table and protocol handler
	c86trm.a86 - terminal mode routines
	c86utl.a86 - CP/M-86 functions and utility routines

The machine-dependent modules are:
	c86xap.a86 - NEC APC
	c86xfj.a86 - Fujitsu Micro 16s
	c86xfu.a86 - Future Computers FX20/FX30
	c86xr2.a86 - DEC Rainbow (for version 2.9 - supports baud rates)
	c86xrb.a86 - DEC Rainbow (for version 2.8)
	c86xtx.a86 - Tektronix 4170
	c86xv9.a86 - Victor 9000

  NOTE: The rest of this file, as well as the manual, refer to some older
        file naming conventions.  In any case, it necessary to assemble the
        six system-independent modules and the desired system-dependent 
        module, and then to link them together.
        
------------------------------

When implementing Kermit on your system, download all the machine-independent
modules and the 86KERIO module for your system.  Then perform the following
procedure to assemble a new command file.

	REN 86KERIO.A86=86KERIO.???	; Where ??? represents your system
	ASM86 86KERMIT $PZ		; Assemble the source with no listing
	GENCMD 86KERMIT			; Generate a CMD file
	REN KERMIT.CMD=86KERMIT.CMD	; Make the CMD file real

Some of this work may be done for you already.  The H86 and CMD files for
the Rainbow are available as RBKERMIT.H86 and RBKERMIT.CMD, and the APC
files are available as APCKERMIT.H86 and APCKERMIT.CMD.  If you are able
to download the H86 file, then you need only perform the last two steps.
If you can do binary command file transfer, then you can obtain the CMD
file directly and need only perform the final renaming step to make your
Kermit operational.

The system dependencies in the 86KERIO file take several forms.  The major
difference between machines is the location and operation of the communication
port.  The other main difference is the method of screen control and
cursor positioning employed.  If there is to be terminal emulation performed,
then that code should also be placed in this module.

Much effort has been made to isolate the major portion of Kermit from these
dependencies, so that all operations are performed by calling machine-dependent
routines according to a specified interface.  The routines that are required
to be present in the 86KERIO module are:

* For port handling *

prtout - sets the parity bit if necessary and outputs the character in AL
	to the currently selected serial port.  All registers are preserved.

instat - returns to the "skip" location (3 bytes following its call) if
	a character is ready to be read from the currently selected serial
	port, otherwise simply returns.  All registers are preserved.

inchr - returns in AL the next character read from the currently selected
	serial port.  All other registers are preserved.

cfibf - clears the serial port input buffer, to get rid of extra NAK packets
	and other random garbage.  All registers are preserved.

prtbrk - signals a break on the currently selected serial port.  No registers
	are preserved.

serini - performs all system-dependent initialization of interrupt vectors,
	port characteristics, and screen handling.  No registers are preserved.

serfin - performs de-initialization to clean up the environment when leaving
	Kermit.  No registers are preserved.

bdset - the SET subcommand to change baud rates on the serial port.  This
	need not be implemented, and in fact is not on the Rainbow.  No
	registers are preserved.

prtset - the SET subcommand to select the communications port for subsequent
	use.  This is not currently implemented on either the APC or the
	Rainbow.  No registers are preserved.

shobd - called from the SHOW routine to display the current baud rate value.
	No registers are preserved.

shoprt - called from the SHOW routine to display the current port selection
	value.  No registers are preserved.


* For screen control *

poscur - positions the cursor to a specified row and column.  The offset in DX
	points to two consecutive one-byte locations containing the new row
	and column, respectively.  No registers are preserved.

clrscr - homes the cursor to row 1 and column 1, then clears the screen.  No
	registers are preserved.

clrlin - returns the cursor to the left margin and clears the current cursor
	line.  No registers are preserved.

clreol - clears from the cursor location to the end of the current cursor line.
	No registers are preserved.

revon - turns on the reverse video attribute, if available.  No registers are
	preserved.

revoff - turns off the reverse video attribute.  No registers are preserved.

bldon - turns on the bold or highlighting attribute, if available.  No
	registers are preserved.

bldoff - turns off the bold attribute.  No registers are preserved.

dotab - positions the cursor to the next tab stop, blanking intermediate
	columns.  Just return "skip" if the system hardware handles this when
	a tab character is displayed, otherwise perform the necessary functions
	and return.  No registers are preserved.

delstr - this is a string of bytes to be displayed during command entry
	whenever a DEL character (7F hex) is keyed in.  Its net effect should
	be to cause the DEL to act as if a backspace had been typed, that is,
	to move the cursor left one column.  This string will vary depending
	on whether the DEL character causes an echo to the screen.

system - this is a string of bytes to identify the system being used.  It
	is displayed in the screen header followed by the string " CP/M-86
	Kermit-86 - Vx.x"

