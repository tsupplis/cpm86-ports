SUBDIRS=ansi2kr tinybas lispc xlisp yacc vtl2 filer dc grep cpm80 lbr \
    edlin

all:
	for d in $(SUBDIRS); do \
		$(MAKE) -C $$d $@; \
	done

clean:
	for d in $(SUBDIRS); do \
		$(MAKE) -C $$d $@; \
	done

test: cpmtest.img
	./cpm86 

cpmtest.img:  all
	cp cpmbase.img $@
	for i in */*.cmd;do cpmcp -f ibmpc-514ss $@ $$i 0:;done
	cpmls -F -f ibmpc-514ss $@ '0:*.*'



.PHONY: all clean $(SUBDIRS)
