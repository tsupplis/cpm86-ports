TPC=cpm86_tp3
RM=rm -f

all: ladder.cmd

ladder.cmd: ladder.pas
	$(TPC) ladder.pas
	$(CMDINFO) $@

clean:
	$(RM) ladder.cmd
