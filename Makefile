# http://www.freesoftwaremagazine.com/articles/gnu_coding_standards_applied_to_autotools

## Default target
DEFAULT=all

## The default target is the first target in the makefile
$(DEFAULT):

## Export all variables to sub-makefiles
export

include Makefile.in

tarname = $(package)
distdir = $(abspath $(tarname)-$(version))

## Targets asked for running, or default target if none
TARGETS=$(if $(MAKECMDGOALS),$(MAKECMDGOALS),$(DEFAULT))

# Check make version
VERSION = $(shell make --version|head -1|cut -f 1-2 -d ' ')

## Make all targets called dependant on some action to be run beforehand
$(TARGETS): $(FIRST)

all check install uninstall: version submake

$(abspath $(distdir)).tar.gz: $(abspath $(distdir))
	tar -ch -C $(abspath $(distdir)) -O .| gzip -9 -c > $(abspath $(distdir)).tar.gz
	
$(abspath $(distdir)): submake
	cp Makefile $(abspath $(distdir))
	cp Makefile.in $(abspath $(distdir))
	
clean: submake clean-tree clean-dist

clean-tree:
	
clean-dist:
	$(RM) -r $(abspath $(distdir))
	$(RM) $(abspath $(distdir)).tar.gz

dist: $(abspath $(distdir)).tar.gz

distcheck: checkdist clean-dist

checkdist: $(abspath $(distdir)).tar.gz
	gzip -cd $+ | tar xvf -
	$(MAKE) -C $(abspath $(distdir)) check
	$(MAKE) -C $(abspath $(distdir)) DESTDIR=$(abspath $(distdir))/_inst install uninstall
	$(MAKE) -C $(abspath $(distdir)) clean

version:
	$(if $(findstring GNU Make,echo $(VERSION)),,@echo This Makefile requires GNU make)
	$(if $(findstring GNU Make,echo $(VERSION)),,@/bin/false)

submake:
	@$(shell echo for i in "$(foreach var,$(subdirs),$(var))"\; do $(MAKE) -C \$$i $(MAKECMDGOALS) distdir=$(distdir)/\$$i\; done)

FORCE:
.PHONY: FORCE all version clean dist distcheck copy clean-dist clean-tree
.PHONY: install uninstall submake
