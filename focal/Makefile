CC = aztec43_acc
LD = aztec43_acc
CFLAGS = -target=cpm86 -I. -squeeze
LDFLAGS = -target=cpm86 -lm

OBJS = focal0.o focal1.o focal2.o focal3.o

.PHONY = all clean

all: focal.cmd

$(OBJS): focal.h proto.h

focal.cmd: $(OBJS)
	$(LD) -o $@ $(OBJS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o *.com *.cmd *.lib
