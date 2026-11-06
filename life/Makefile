CC=aztec43_acc
LD=aztec43_acc
CFLAGS=-I. -squeeze
CPM_LDFLAGS=-target=cpm86
DOS_LDFLAGS=-target=dos1
RM=rm -rf

all: binaries

OBJS=life.o

binaries: life.cmd life.com

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

life.com: $(OBJS)
	$(LD) $(DOS_LDFLAGS) -o $@ $(OBJS)

life.cmd: $(OBJS)
	$(LD) $(CPM86_LDFLAGS) -o $@ $(OBJS)

clean:
	$(RM) *.o *.cmd *.com
