XLT86=cpm_xlt86
ASM86=cpm86_asm86
GENCMD=cpm86_gencmd
CMDINFO=cmdinfo

all: wash15.cmd

# 8080 model; the copy buffer runs from the end of the file list to the top
# of the segment: at least 16K, up to 64K
wash15.cmd: wash15.h86
	$(GENCMD) $< 8080 'CODE[M400,XFFF]'
	$(CMDINFO) $@

wash15.h86: wash15.a86
	$(ASM86) $(basename $<)

# 8080 model; the copy buffer runs from the end of the program to the top
# of the segment: at least 16K, up to 64K
orig.cmd: orig.h86
	$(GENCMD) $< 8080 'CODE[M400,XFFF]'
	$(CMDINFO) $@

orig.h86: orig.a86
	$(ASM86) $(basename $<)

# wash15.a86 is orig.a86, regenerated from the original 8080 source with:
#	make orig.a86 && cp orig.a86 wash15.a86

# XLT86 stops with "Fatal Error: Not BDOS" on any CALL to a routine named
# CDEHL (or CDE): the name, not the code, trips it. LIST, LOOP and ESC are
# ASM86 reserved words or 8086 instructions. Rename them on the way in.
#	perl -pe 's/\bCDEHL\b/CMPDEHL/g; s/\bLIST\b/FNLIST/g; s/\bLOOP\b/WLOOP/g; s/\bESC\b/ESCCHR/g' $< > $@

# The patch holds the CP/M-86 fixes XLT86 cannot make: own stack segment
# with interrupts back on, the logged drive from BDOS 25 instead of 0004h,
# and the inline PNAME buffer read through DI. XLT86 writes CR/LF and ^Z
# padding: strip them for patch and restore CR/LF for ASM86. Its output
# must not be redirected.
orig.a86: orig.asm orig.a86.patch
	rm -f $@ orig.prn
	$(XLT86) $(basename $<)
	@if grep -q "Error" orig.prn; then tr -d '\r\032' < orig.prn; exit 1; fi
	tr -d '\r\032' < $@ > $@.tmp && mv $@.tmp $@
	patch -s $@ < orig.a86.patch
	unix2dos -q $@

clean:
	rm -f orig.a86 orig.a86.tmp orig.a86.orig orig.prn orig.cmd \
		'orig.$$$$' wash15.cmd wash15.h86 wash15.sym wash15.lst 
