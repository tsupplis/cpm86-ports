PYTHON=python3
XLT86=cpm_xlt86
ASM86=cpm86_asm86
GENCMD=cpm86_gencmd
CMDINFO=cmdinfo

all: disk7.cmd

# 8080 model; the copy buffer runs from the end of the program to the top
# of the segment: at least 16K, up to 64K
disk7.cmd: disk7.h86
	$(GENCMD) $< 8080 'CODE[M400,XFFF]'
	$(CMDINFO) $@

disk7.h86: disk7.a86
	$(ASM86) $(basename $<)

# 8080 model; the copy buffer runs from the end of the program to the top
# of the segment: at least 16K, up to 64K
orig.cmd: orig.h86
	$(GENCMD) $< 8080 'CODE[M400,XFFF]'
	$(CMDINFO) $@

orig.h86: orig.a86
	$(ASM86) $(basename $<)

# XLT86 has no macros, ELSE, etc.: prep80.py turns the original DR MAC
# source into plain 8080 source first
orig.asm: orig.mac prep80.py
	$(PYTHON) prep80.py $< > $@

# The patch holds the CP/M-86 fixes XLT86 cannot make: own stack segment,
# exit through BDOS 0, console input without the BIOS jump table, and ES:BX
# results of BDOS 27/31. XLT86 writes CR/LF and ^Z padding: strip them for
# patch and restore CR/LF for ASM86. Its output must not be redirected.
orig.a86: orig.asm orig.a86.patch
	rm -f $@ orig.prn
	$(XLT86) $(basename $<)
	@if grep -q "Error" orig.prn; then tr -d '\r\032' < orig.prn; exit 1; fi
	tr -d '\r\032' < $@ > $@.tmp && mv $@.tmp $@
	patch -s $@ < orig.a86.patch
	unix2dos -q $@

clean:
	rm -f orig.asm orig.a86 orig.a86.tmp orig.a86.orig \
		orig.h86 orig.lst orig.sym orig.prn orig.cmd \
		'orig.$$$$' disk7.cmd disk7.h86 disk7.sym disk7.lst
