SUBDIRS=ansi2kr tinybas lispc yacc vtl2

all clean:
	for d in $(SUBDIRS); do \
		$(MAKE) -C $$d $@; \
	done

.PHONY: all clean $(SUBDIRS)
