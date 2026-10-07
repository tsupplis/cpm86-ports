CC=aztec42_cc
LD=aztec42_link
CFLAGS=
CPM_LDFLAGS=-lc86
DOS_LDFLAGS=-lc

all: binaries

OBJS=lisp.o list.o operator.o parser.o primtivs.o printer.o

binaries: lispc.com lispc.cmd

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

lispc.com: $(OBJS)
	$(LD) -o $@ $(OBJS) $(DOS_LDFLAGS)

lispc.cmd: $(OBJS)
	$(LD) -o $@ $(OBJS) $(CPM_LDFLAGS)

clean:
	rm -rf *.o *.cmd *.com
