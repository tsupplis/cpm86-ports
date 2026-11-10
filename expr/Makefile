CC = aztec43_acc
LD = aztec43_acc
LIB = aztec43_lib
CFLAGS = -target=cpm86 -I. -squeeze
LDFLAGS = -target=cpm86 -lm87s

.PHONY = all clean

all: expr.cmd tinyexpr.lib 

# --------------------------------------------------------------------
# Library
# --------------------------------------------------------------------
tinyexpr.lib: tinyexpr.o
	rm -f tinyexpr.lib
	$(LIB) tinyexpr.lib tinyexpr.o

# --------------------------------------------------------------------
# Binaries
# --------------------------------------------------------------------

smoke.cmd: smoke.o tinyexpr.lib
	$(LD) -o $@ smoke.o tinyexpr.lib $(LDFLAGS)

expr.cmd: expr.o tinyexpr.lib
	$(LD) -o $@ expr.o tinyexpr.lib $(LDFLAGS)

reg.cmd: reg.o
	$(LD) -o $@ reg.o $(LDFLAGS)

expl.cmd: expl.o tinyexpr.lib
	$(LD) -o $@ expl.o tinyexpr.lib $(LDFLAGS)

expl2.cmd: expl2.o tinyexpr.lib
	$(LD) -o $@ expl2.o tinyexpr.lib $(LDFLAGS)

expl3.cmd: expl3.o tinyexpr.lib
	$(LD) -o $@ expl3.o tinyexpr.lib $(LDFLAGS)

# --------------------------------------------------------------------
# Compile rules
# --------------------------------------------------------------------
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

test: expl.cmd expl2.cmd expl3.cmd smoke.cmd reg.cmd
	emu2 expl.cmd
	emu2 expl2.cmd "45+sqrt(34)"
	emu2 expl3.cmd
	emu2 smoke.cmd
	emu2 reg.cmd

clean:
	rm -f *.o *.com *.cmd *.lib
