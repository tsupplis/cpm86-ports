CC=aztec43_acc
LD=aztec43_acc
CFLAGS=-target=cpm86 -I. -squeeze
CPM_LDFLAGS=-target=cpm86
CMDMOD=cmdmod
PYTHON=python3

all: sokoban.cmd

OBJS=sokoban.o gameio.o resource.o help.o
$(OBJS): Makefile

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# util.lib and its headers are local snapshots supplied from cpm86-hacking.
sokoban.cmd: $(OBJS) util.lib res.bin Makefile
	$(LD) $(CPM_LDFLAGS) -o soknew.cmd $(OBJS) util.lib
	$(CMDMOD) -t EXTRA -s res.bin -m "$$(printf '%x' $$(wc -c < res.bin))" soknew.cmd 2
	mv soknew.cmd $@

res.bin: soklevls.dat sokoban.hlp packres.py
	$(PYTHON) packres.py soklevls.dat sokoban.hlp $@

sokoban.o: config.h gameio.h conio.h resource.h help.h
gameio.o: gameio.h conio.h util.h
resource.o: resource.h
help.o: resource.h help.h gameio.h conio.h

test: sokoban.cmd test-keys
	SK_TEST_CC="$(CC)" SK_TEST_CMDMOD="$(CMDMOD)" SK_TEST_MAKE="$(MAKE)" $(PYTHON) test.py

clean:
	rm -rf *.o *.cmd
	rm -f res.bin

.PHONY: all test clean test-keys
test-keys:
	python3 testkeys.py
