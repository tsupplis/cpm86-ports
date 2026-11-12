CC=aztec43_acc
LD=aztec43_acc
CMDMOD=cmdmod
PYTHON=python3
CFLAGS=-target=cpm86 -I. -squeeze
LDFLAGS=-target=cpm86

BINARIES=yacc.cmd
OBJS=y1.o y2.o y3.o y4.o template.o

all: binaries

binaries: $(BINARIES) 

yacc.cmd: $(OBJS) tpl.bin Makefile
	$(LD) $(LDFLAGS) -o yaccnew.cmd $(OBJS)
	$(CMDMOD) -t EXTRA -s tpl.bin -m "$$(printf '%x' $$(wc -c < tpl.bin))" yaccnew.cmd 2
	mv yaccnew.cmd $@

tpl.bin: yaccpar.tpl packtpl.py
	$(PYTHON) packtpl.py $< $@

y1.o y2.o y3.o y4.o: dextern.h files.h
y1.o template.o: template.h

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	$(RM) *.o *.h86 *.log *.sym *.prn *.lst *.obj $(BINARIES) yaccnew.cmd tpl.bin

test: yacc.cmd
	YACC_TEST_CC="$(CC)" YACC_TEST_CMDMOD="$(CMDMOD)" YACC_TEST_MAKE="$(MAKE)" $(PYTHON) test.py

.PHONY: all binaries clean test
