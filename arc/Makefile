CC=aztec43_acc
LD=aztec43_acc
CMDINFO=pcdev_cmdinfo
CFLAGS=-target=cpm86 -I. -squeeze
LDFLAGS=-target=cpm86 -L. -lutil

BINARIES=arc86.cmd marc86.cmd

OBJS=arc.o arcadd.o arccode.o arccvt.o arcdata.o arcdel.o arcdos.o \
	arcext.o arcio.o arclst.o arclzw.o arcmatch.o arcpack.o arcrun.o \
	arcsq.o arcsvc.o arctst.o arcunp.o arcusq.o arcmisc.o

MOBJS=marc.o arcdata.o arcdos.o arcio.o arcmatch.o arcmisc.o

all: binaries

binaries: $(BINARIES)

arc86.cmd: $(OBJS) util.lib
	$(LD) -o $@ $(OBJS) $(LDFLAGS)
	$(CMDINFO) $@

marc86.cmd: $(MOBJS) util.lib
	$(LD) -o $@ $(MOBJS) $(LDFLAGS)
	$(CMDINFO) $@

%.o: %.c arc.h arcs.h proto.h util.h dirent.h
	$(CC) $(CFLAGS) -c -o $@ $<

test: binaries
	sh test.sh
	python3 test-drive.py
	$(MAKE) -C ../util test

clean:
	$(RM) *.o *.h86 *.log *.sym *.prn *.lst *.obj $(BINARIES)

.PHONY: all binaries test clean

# Shared portable file replacement implementation.
arcmisc.o: ../util/fsave.h
