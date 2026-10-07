CC=aztec42_cc
LD=aztec42_link
CFLAGS=-I. -DAZTEC
CPM_LDFLAGS=-lc86

all: sokoban.cmd

OBJS=sokoban.o conio.o

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

sokoban.cmd: $(OBJS)
	$(LD) -o $@ $(OBJS) $(CPM_LDFLAGS)

test: sokoban.cmd

clean:
	rm -rf *.o *.cmd
