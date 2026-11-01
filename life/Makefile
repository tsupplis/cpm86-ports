CC=aztec42_cc
LD=aztec42_link
STRIP=aztec42_sqz
CFLAGS=
CPM_LDFLAGS=-lc86
DOS_LDFLAGS=-lc
RM=rm -rf

all: binaries

OBJS=life.o

binaries: life.cmd life.com

%.o: %.c
	$(CC) $(CFLAGS) -o $@ $<
	$(STRIP) $@

life.com: $(OBJS)
	$(LD) -o $@ $(OBJS) $(DOS_LDFLAGS)

life.cmd: $(OBJS)
	$(LD) -o $@ $(OBJS) $(CPM_LDFLAGS)

clean:
	$(RM) *.o *.cmd *.com
