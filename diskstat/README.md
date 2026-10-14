MML Diskstat                                          John Elliott, 9 May 2015
# Synopsis

DISKSTAT is a program to display the characteristics of a disk drive under 
CP/M: its DPB (Disk Parameter Block), DPH (Disk Parameter Header) and usage 
statistics (used / free blocks).

- DSTAT86.CMD: DISKSTAT 1.2 (8086): XLT86 was used to convert DISKSTAT to 8086
            instructions, and the source was tidied up manually. Supports the
            following CP/M-86 versions: CP/M-86 2.2, CP/M-86 Plus, Personal
            CP/M-86, Concurrent CP/M-86 3.1, Concurrent CP/M-86 4.1, DOS Plus
            1.2 and DOS Plus 2.1. 

- DSTAT86.A86   for NASM. NASM cannot directly output CP/M-86 CMD files, so 
              the utility bin2cmd has been supplied to generate them.

## Customisation

The builds of DISKSTAT 1.2 can be customised, by editing a 12-byte 
area of the program. This is located at:

DSTAT86.CMD: 01BEh   (013Eh when loaded)

  The area is laid out:

```
	DB	'.'	;Character used to represent empty space
	DB	'#'	;Character used to represent directory blocks
	DB	'+'	;Character used to represent data blocks
	DB	'-'	;Character used to represent erased blocks
	DS	8	;Clear screen escape sequence, terminated with '$'
			;If it is just '$', the screen will be cleared by
			;writing linefeeds to it.
```

  For example, on the Amstrad PCW, the area could be set to:

```
	DB	90h			;Small dot
	DB	80h			;Block
	DB	0BCh			;Solid bullet
	DB	0BBh			;Hollow bullet
	DB	27,'E',27,'H','$'	;Clear screen and home cursor
```


