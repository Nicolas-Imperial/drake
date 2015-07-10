## Fills configure directory variables with default values if necessary and define environment variables
final_prefix=$(if $(prefix),$(prefix),$(HOME)/.local)
final_libdir=$(if $(libdir),$(libdir),$(prefix)/lib)
final_includedir=$(if $(includedir),$(includedir),$(prefix)/include)
final_bindir=$(if $(bindir),$(bindir),$(prefix)/bin)

## Default target
DEFAULT=all

## The default target is the first target in the makefile
$(DEFAULT):

## Export all variables to sub-makefiles
export

include Makefile.in

## Targets asked for running, or default target if none
TARGETS=$(if $(MAKECMDGOALS),$(MAKECMDGOALS),$(DEFAULT))

## Make all targets called dependant on some action to be run beforehand
$(TARGETS): $(FIRST)

all clean: submake 

dist: submake
	mkdir -p $(distdir)
	cp Makefile $(distdir)
	cp Makefile.in $(distdir)
	$(if $(strip $(attach)),cp $(attach) $(distdir))
	
install: pre-install do-install post-install submake

pre-install:

do-install:
	$(if $(strip $(attach)),install -d $(DESTDIR)$(prefix)/$(subdir))
	$(if $(strip $(attach)),install -m 644 $(attach) $(DESTDIR)$(prefix)/$(subdir))

post-install:
	
uninstall: submake
	$(RM) $(foreach var,$(attach), $(DESTDIR)$(prefix)/$(subdir)/$(var))
	
check: submake
	$(shell echo for i in "$(foreach var,$(subdirs),$(var))"\; do $(MAKE) -C \$$i $@ \; done)

checkdist distcheck: dist

submake:
	@$(shell echo for i in "$(foreach var,$(subdirs),$(var))"\; do $(MAKE) -C \$$i $(MAKECMDGOALS) distdir=$(distdir)/\$$i subdir=$(subdir)/\$$i\|\|exit 1\; done)
        
.PHONY: all dist install pre-install do-install post-install check submake
