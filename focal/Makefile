CC = aztec43_acc
LD = aztec43_acc
CFLAGS = -I. -squeeze
CPM_LDFLAGS = -target=cpm86 -lm
DOS_LDFLAGS = -target=dos1 -lm

OBJS = focal0.o focal1.o focal2.o focal3.o
CMDS = focal.cmd focal.com

.PHONY = all clean

all: $(CMDS)

$(OBJS): focal.h proto.h

focal.cmd: $(OBJS)
	$(LD) -o $@ $(OBJS) $(CPM_LDFLAGS)

focal.com: $(OBJS)
	$(LD) -o $@ $(OBJS) $(DOS_LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o *.com *.cmd *.lib
