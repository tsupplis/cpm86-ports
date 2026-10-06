CC=aztec42_cc
LD=aztec42_link
CFLAGS=

all: binaries

OBJECTS=tinybasic.o common.o errors.o expression.o formatter.o generatec.o interpret.o \
options.o parser.o statement.o token.o tokeniser.o
CMDS=tinybas.com tinybas.cmd 
TOYS=lander.cmd lander.com wumpus.cmd wumpus.com mugwump.cmd mugwump.com \
	hurkle.cmd hurkle.com tictacto.cmd tictacto.com hammurab.cmd hammurab.com

binaries: $(CMDS) 

tinybas.com: $(OBJECTS)
	$(LD) -f dos.inp
	[ -f tinybas.com ] || exit 1

tinybas.cmd: $(OBJECTS)
	$(LD) -f cpm86.inp
	[ -f tinybas.cmd ] || exit 1

%.cmd: %.o
	$(LD) -o $@ $^ -lc86

%.com: %.o
	$(LD) -o $@ $^ -lc

%.o: %.c
	$(CC) -c $^ $(CFLAGS)
	
%.c: %.bas 
	$(MAKE) tinybas.com
	emu2 tinybas.com -oc $^

test: binaries $(TOYS)

clean:
	rm -rf *.o *.com *.cmd lander.c mugwump.c wumpus.c hurkle.c

