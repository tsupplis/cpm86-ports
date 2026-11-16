TPC=cpm86_tp3
CMDINFO=pcdev_cmdinfo
RM=rm -f

all: ladder.cmd

ladder.cmd: $(filter-out tsttime.pas,$(wildcard *.pas))
	$(TPC) ladder.pas
	$(CMDINFO) $@

clean:
	$(RM) ladder.cmd
