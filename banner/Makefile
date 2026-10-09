CC=aztec42_cc
LD=aztec42_link
CFLAGS=
CPM_LDFLAGS=-lc86
DOS_LDFLAGS=-lc

all: binaries

OBJS=banner.o

binaries: banner.cmd banner.com

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

banner.com: $(OBJS)
	$(LD) -o $@ $(OBJS) $(DOS_LDFLAGS)

banner.cmd: $(OBJS)
	$(LD) -o $@ $(OBJS) $(CPM_LDFLAGS)

clean:
	rm -rf *.o *.cmd *.com
