## Enable GNU make secondary expansion feature
.SECONDEXPANSION:

## Export all variables to sub-makefiles
export

## Fills configure directory variables with default values if necessary and define environment variables
final_prefix=$(if $(prefix),$(prefix),$(HOME)/.local)
final_libdir=$(if $(libdir),$(libdir),$(prefix)/lib)
final_includedir=$(if $(includedir),$(includedir),$(prefix)/include)
final_bindir=$(if $(bindir),$(bindir),$(prefix)/bin)

export LIBRARY_PATH := $(LIBRARY_PATH):$(final_libdir)
export LD_LIBRARY_PATH := $(LIBRARY_PATH):$(final_libdir)
export PKG_CONFIG_PATH := $(PKG_CONFIG_PATH):$(final_libdir)/pkgconfig
export C_INCLUDE_PATH := $(C_INCLUDE_PATH):$(final_includedir)
export CPLUS_INCLUDE_PATH := $(CPLUS_INCLUDE_PATH):$(final_includedir)

## Make this folder as a pkgconfig search path so locally compiled libraries can be resolved before installation
export PKG_CONFIG_PATH:=.:$(PKG_CONFIG_PATH)

## Default target
DEFAULT=all

## Targets asked for running, or default target if none
TARGETS=$(if $(MAKECMDGOALS),$(MAKECMDGOALS),$(DEFAULT))

## The default target is the first target in the makefile
$(DEFAULT):
include Makefile.in

## Make all targets called dependent on some action to be run beforehand
$(TARGETS): $(FIRST)

####################################################################
##### Here begin the region you really don't want to mess with #####
####################################################################

snekkja_schedule = schedule

## Gives the longest prefix in the prefix list (all elements must have the same character at the same index, or no character at that index)
define longest
$(if $(strip $1),$(word $(words $1),$(sort $1)),)
endef

## Gives the source file extension of language $1
define ext
$(if $(filter cc,$1),c,$(if $(filter cxx,$1),cpp,))
endef

## Finds the longest suffix of $1 in the list $2
define suffixin
$(firstword $(shell echo $(sort $(shell echo $(foreach var,$2,$(if $(filter %$(var),$1),$(var),))|rev))|rev))
endef

## Get pkg-config output for the operation $2 (cflags|libs) on package list $1, with the corresponding options from variable $3$<package_name>
define pkg-config
$(if $(strip $1),$(if $(shell PKG_CONFIG_PATH="$(PKG_CONFIG_PATH)" pkg-config --$2 $(foreach pkg,$1,"$(pkg) $(call $3$(pkg)_version)")),$(shell PKG_CONFIG_PATH="$(PKG_CONFIG_PATH)" pkg-config --$2 $(foreach pkg,$1,"$(pkg) $(call $3$(pkg)_version)")),$(error Missing library $3)),)
endef

## Gives all source files of projects $1 of type $2 (bin|lib|test|shared) with language $3 (cc|cxx)
define src
$(foreach type,cc cxx,$(foreach var,$1,$(call $2$(type)_$(var)_src)))
endef

## Gives source files of projects $1 of type $2 (bin|lib|test|shared) with language $3 (cc|cxx), prefixed with their file variant
define src_file_var
$(foreach var,$1,$(foreach src,$(call $2$3_$(var)_src),$(call VARIANT_$(src))$(src)))
endef

## Gives source files of projects $1 of type $2 (bin|lib|test|shared) with language $3 (cc|cxx), prefixed with their project and file variant
define src_project_file_var
$(foreach var,$(call src_file_var,$1,$2,$3),$(call VARIANT_$1_$(call source,$(patsubst %.$(call typeof,$1),%.o,$(var)),$1,$2,$3))$(var))
endef

## Gives object files of projects in list $1, of type $2 (bin|lib|test|shared) with language $3 (cc|cxx), prefixed with their project name, project type, language $4 (if any, otherwise $3) and source file variant
define object
$(foreach proj,$1,$(patsubst %.$(call ext,$3),$2$(if $4,$4,$3)_$(proj)_%.o,$(call src_project_file_var,$(proj),$2,$3)))
endef

## Same as above, for snekkja projects
define kkj_object
$(foreach proj,$1,$(if $(call $2$3_$(proj)_graph),$(foreach task,$(shell snekkja --taskgraph $(call $2$3_$(proj)_graph) --get tasks),$2$3_$(proj)_$(task).o))) $(foreach proj,$1,$(foreach src,$(call kkj_schedule_src,$(proj),$3),$2$3_$(patsubst %.$(call ext,$3),%,$(src)).o))
endef

## Gives object files of projects in list $1, of type $2 (bin|lib|test) with language $3 (cc|cxx), prefixed with their project name, project variant and source file variant
define embed
$(foreach proj,$1,$(patsubst %,%.o,$(call $2_$(proj)_embed) $(if $(patsubst kkj,$2,$2),$(if $(call $2cc_$(proj)_graph),$(call $2cc_$(proj)_graph),$(call $2cxx_$(proj)_graph)) $(call $2_$(proj)_platform))))
endef

## Gives the source files of the embedded object files in $1
define embed_src
$(foreach var,$1,$(patsubst %.o,%,$(var)))
endef

## Gives object files of projects $1 of type $2 (bin|lib|test) with language $3 (cc|cxx), prefixed with their project name, but no project or source file variant
define object_noprefix
$(foreach proj,$1,$(patsubst %.$(call ext,$3),$2_$(proj)_%.o,$(call src,$(proj),$2,$3)))
endef

## Gives the project of the project-prefixed object file $1 of type $2 (bin|lib|test) and language $3 (cc|cxx)
define projectof
$(call longest,$(foreach proj,$(call $2$(if $(filter bin,$2),$3,$(if $(filter test,$2),$3,$(if $(filter shared,$2),$3,)))_package),$(foreach lang,cc cxx,$(if $(filter $2$(lang)_$(proj)%,$1),$(proj),))))
endef

## Gives the type of project (bin|lib|test) the object file $1 is part of
define typeof
$(if $(filter bin%,$1),bin,$(if $(filter lib%,$1),lib,$(if $(filter test%,$1),test,$(if $(filter kkj%,$1),kkj,$(if $(filter shared%,$1),shared,)))))
endef

## Gives the project corresponding the the kkj object file $1 in language $2
define kkj_project
$(foreach proj,$(kkj_package),$(if $(filter kkj$2_$(proj)_%.o,$1),$(proj)))
endef

## Gives the taskid corresponding the the kkj object file $1 in language $2 of project $3
define kkj_task
$(if $(filter kkj$2_$3%.o,$1),$(patsubst kkj$2_$3_%.o,%,$1))
endef

## Gives the source file of object $1 of project $2 of type $3 (bin|lib|test) in language $4 (cc/cxx)
define source
$(if $(filter kkj%.o,$1),$(if $(filter %.$(snekkja_schedule).o,$1),$(patsubst $3$4_%.$(snekkja_schedule).o,%.$(snekkja_schedule).$(call ext,$4),$1),$(if $(call $3$4_$(sort $2)_$(shell snekkja --taskgraph $(call $3$4_$(sort $2)_graph) --get task $(call kkj_task,$1,$4,$(sort $2)))),$(call $3$4_$(sort $2)_$(shell snekkja --taskgraph $(call $3$4_$(sort $2)_graph) --get task $(call kkj_task,$1,$4,$(sort $2)))),$(shell snekkja --taskgraph $(call $3$4_$(sort $2)_graph) --get task $(call $3_task,$1,$4,$(sort $2))).$(call ext,$4))),$(call suffixin,$(patsubst %.o,%.$(call ext,$4),$1),$(call src,$(sort $2),$3,$4)))
endef

# Returns the package main language of object $1 of project type $2 (bin|lib|test)
define langof
$(foreach lg,cc cxx,$(filter $(lg),$(patsubst $2$(lg)_%,$(lg),$1)))
endef

## Gives all makefile-wise cflags
## Overridable global CFLAG variable
## Base CFLAGS from MAKEFILE
## pkg-config CFLAGS
## Global DEBUG/RELEASE CFLAGS
## No need of a function here

GLOBAL_CFLAGS_PRIVATE = $(CFLAGS_PRIVATE) $(CFLAGS) $(call pkg-config,$(CC_PKG),cflags,CC_PKG_) $(call CFLAGS_CONFIG_$(CONFIG))
GLOBAL_CFLAGS_EXPORT = $(CFLAGS_BASE)
GLOBAL_CFLAGS = $(GLOBAL_CFLAGS_PRIVATE) $(GLOBAL_CFLAGS_EXPORT)
GLOBAL_CXXFLAGS_PRIVATE = $(CXXFLAGS_PRIVATE) $(CXXFLAGS) $(call pkg-config,$(CXX_PKG),cflags,CXX_PKG_) $(call CXXFLAGS_CONFIG_$(CONFIG))
GLOBAL_CXXFLAGS = $(GLOBAL_CXXFLAGS_PRIVATE) $(GLOBAL_CFLAGS_EXPORT)

## Gives cflags for project $1 of type $2 (bin|lib|test) in language $3 (cc|cxx), that will not be exported in pkg-config
define cflags_project_private
$(call pkg-config,$(call $2_$1_pkg),cflags,$2_$1_pkg_)
endef

## Gives cflags for project $1 of type $2 (bin|lib|test) in language $3 (cc|cxx), that will be exported in pkg-config
define cflags_project_export
$(call $2_$1_cflags)
endef

## Gives cflags for project $1 of type $2 (bin|lib|test) in language $3 (cc|cxx)
define cflags_project
$(if $(filter shared,$2),$(if $(filter $3,cc),$(CFLAGS_SHARED),$(if $(filter $3,cxx),$(CXXFLAGS_SHARED)))) $(call cflags_project_private,$1,$2,$3) $(call cflags_project_export,$1,$2,$3)
endef

define prefix_flags
$(if $(filter cxx,$1),cxx,$(if $(filter cc,$1),c,))
endef

## Gives the private cflags of the project corresponding to object file $1
define cflags_private_object
$(foreach lang,cc cxx,$(if $(call projectof,$1,$(call typeof,$1),$(lang)),$(call $(call typeof,$1)_$(call projectof,$1,$(call typeof,$1),$(lang))_$(call prefix_flags,$(lang))flags_private),))$(if $(filter kkj%,$1),$(if $(filter %.$(snekkja_schedule).o,$1),-DAPP_NAME=$(foreach lang,cc cxx,$(if $(filter kkj$(lang)_%.$(snekkja_schedule).o,$1),$(patsubst kkj$(lang)_%.$(snekkja_schedule).o,%,$1))),-D_SNEKKJA_COMPILE -DTASK_NAME=$(strip $(foreach lang,cc cxx,$(if $(call kkj_project,$1,$(lang)),$(shell snekkja --taskgraph $(call kkj$(lang)_$(call kkj_project,$1,$(lang))_graph) --get task $(call kkj_task,$1,$(lang),$(call kkj_project,$1,$(lang))))))) -DTASK_ID=$(strip $(foreach lang,cc cxx,$(call kkj_task,$1,$(lang),$(call kkj_project,$1,$(lang)))))))
endef

GLOBAL_CC_LDFLAGS=$(CC_LDFLAGS) $(call pkg-config,$(CC_PKG),libs-only-L,CC_PKG_)
GLOBAL_CXX_LDFLAGS=$(CXX_LDFLAGS) $(call pkg-config,$(CXX_PKG),libs-only-L,CXX_PKG_)

define ldflags_project
$(call $2_$1_ldflags) $(call pkg-config,$(call $2_$1_pkg),libs-only-L $(if $(strip $(call $2_$1_static)),$(if $(filter 0,$(strip $(call $2_$1_static))),,--static),),$2_$1_pkg_)
endef

## Gives all cflags the object file $1 is ought to receive for this project of $2 (bin|lib|test) in language "cc"
## The iglobal cflag for this object file
## The project-defined cflag for this object file
define cflags_file
$(call CFLAGS_$(call source,$1,$(call projectof,$1,$2,cc),$2,cc)) $(call $2cc_$(call projectof,$1,$2,cc)_cflags_$(call source,$1,$(call projectof,$1,$2,cc),$2,cc))
endef

## Same as above for C++ projects
define cxxflags_file
$(call CXXFLAGS_$(call source,$1,$(call projectof,$1,$2,cxx),$2,cxx)) $(call $2cxx_$(call projectof,$1,$2,cxx)_cxxflags_$(call source,$1,$(call projectof,$1,$2,cxx),$2,cxx))
endef

## Gives all library switches brought by the makefile and its options
## Global pkg-config packages
## Global manual LIBS variable
GLOBAL_CC_LIBS=$(CC_LIBS)
GLOBAL_CXX_LIBS=$(CXX_LIBS)
GLOBAL_CC_TEST_LIBS=$(CC_TEST_LIBS)
GLOBAL_CXX_TEST_LIBS=$(CXX_TEST_LIBS)
GLOBAL_CC_PKG=$(call pkg-config,$(CC_PKG),libs-only-l,CC_PKG_)
GLOBAL_CXX_PKG=$(call pkg-config,$(CXX_PKG),libs-only-l,CXX_PKG_)
GLOBAL_CC_TEST_PKG=$(call pkg-config,$(CC_TEST_PKG),libs-only-l,CC_TEST_PKG_)
GLOBAL_CXX_TEST_PKG=$(call pkg-config,$(CXX_TEST_PKG),libs-only-l,CXX_TEST_PKG_)

## Gives all library switches of project $1 of type $2 (bin|lib|test) in language $3
## pkg-config provided switches from _pkg variable
## manual library switches from _libs variable
define libs_project
$($2_$(call suffixin,$1,$($2$3_package))_libs)
endef

## Returns all libraries of packages upon which project $1 (type $2 and language $3) depends on.
define pkg_project
$(call pkg-config,$(call $2_$1_pkg),libs-only-l $(if $(strip $(call $2_$1_static)),$(if $(filter 0,$(strip $(call $2_$1_static))),,--static),),$2_$1_pkg_)
endef

## A source filename for snekkja application $1 in language $2
define kkj_schedule_src
$(if $(strip $(call kkj$2_$1_graph)),$1.$(snekkja_schedule).$(if $(filter cc,$2),c,$(if $(filter cxx,$2),cxx)))
endef

## All C and C++ objects, eliminating duplicates
snekkja_schedule_src = $(foreach proj,$(kkj_package),$(foreach lang,cc cxx,$(call kkj_schedule_src,$(proj),$(lang))))
cc_object = $(sort $(call object,$(bincc_package),bin,cc) $(call object,$(bincxx_package),bin,cc,cxx) $(call object,$(lib_package),lib,cc) $(call object,$(testcc_package),test,cc) $(call object,$(testcc_package),test,cc,cxx) $(call kkj_object,$(kkj_package),kkj,cc) $(call object,$(sharedcc_package),shared,cc) $(call object,$(sharedcxx_package),shared,cc,cxx))
cxx_object = $(sort $(call object,$(bincxx_package),bin,cxx) $(call object,$(bincx_package),bin,cxx,cx) $(call object,$(lib_package),lib,cxx) $(call object,$(testcxx_package),test,cxx) $(call object,$(testcc_package),test,cxx,cc) $(call kkj_object,$(kkj_package),kkj,cc) $(call object,$(sharedcxx_package),shared,cxx) $(call object,$(sharedcc_package),shared,cxx,ccx))
lib_pkgconfig = $(sort $(foreach var,$(lib_package) $(kkj_package) $(sharedcxx_package) $(sharedcc_package),$(patsubst %,%.pc,$(var))))
embed_object = $(sort $(call embed,$(bincc_package),bin) $(call embed,$(testcc_package),test) $(call embed,$(bincxx_package),bin) $(call embed,$(testcxx_package),test) $(call embed,$(lib_package),lib) $(call embed,$(kkj_package),kkj) $(call embed,$(sharedcc_package),shared) $(call embed,$(sharedcxx_package),shared))

-include $(foreach file,$(cc_object) $(cxx_object),$(patsubst %.o,%.d,$(file)))

## Build the list of targets this makefile can build
## The list of all c and c++ packages, prefixed all these packages with a specific variant
## variable that may or may not have been defined
bincc_list = $(foreach var,$(bincc_package),$(call VARIANT_$(var))$(var))
bincxx_list = $(foreach var,$(bincxx_package),$(call VARIANT_$(var))$(var))
bin_list = $(bincc_list) $(bincxx_list)

lib_list = $(foreach var,$(lib_package),lib$(call VARIANT_lib$(var).a)$(var).a) $(foreach var,$(kkj_package),lib$(call VARIANT_lib$(var).a)$(var).a)
sharedcc_list = $(foreach lgl,cc,$(foreach var,$(call shared$(lgl)_package),lib$(call VARIANT_shared$(var).so)$(var).so.$(version).$(minor).$(release)))
sharedcxx_list = $(foreach lgl,cxx,$(foreach var,$(call shared$(lgl)_package),lib$(call VARIANT_shared$(var).so)$(var).so.$(version).$(minor).$(release)))
shared_list = $(sharedcc_list) $(sharedcxx_list)
shared_linkername = $(foreach shared,$(shared_list),$(patsubst %.$(version).$(minor).$(release),%,$(shared)))
shared_soname = $(foreach shared,$(shared_list),$(patsubst %.$(version).$(minor).$(release),%.$(version),$(shared)))

testcc_target = $(testcc_package:%=test_%)
testcxx_target = $(testcxx_package:%=test_%)
test_target = $(testcc_package:%=test_%) $(testcxx_package:%=test_%)
testcc_list = $(foreach var,$(testcc_package),test_$(call VARIANT_test_$(var))$(var))
testcxx_list = $(foreach var,$(testcxx_package),test_$(call VARIANT_test_$(var))$(var))
test_list = $(testcc_list) $(testcxx_list)

## Disable implicit rules
.SUFFIXES:

all: submake $(bin_list) $(lib_pkgconfig)

## Force rebuild upon Makefile update
$(cc_object) $(cxx_object) $(embed_object) $(bin_list) $(lib_list) $(shared_list) $(shared_linkername) $(shared_soname) $(lib_pkgconfig): Makefile Makefile.in

## Generate snekkja schedules
$(snekkja_schedule_src): $$(call kkj$$(if $$(filter %.c,$$@),cc,$$(if $$(filter %.cpp,$$@),cxx))_$$(patsubst %.$$(snekkja_schedule).$$(if $$(filter %.c,$$@),c)$$(if $$(filter %.cpp,$$@),cpp),%,$$@)_graph) $$(call kkj_$$(patsubst %.$$(snekkja_schedule).$$(if $$(filter %.c,$$@),c)$$(if $$(filter %.cpp,$$@),cpp),%,$$@)_platform)
	snekkja --get schedule --taskgraph $(call kkj$(if $(filter %.c,$@),cc,$(if $(filter %.cpp,$@),cxx))_$(patsubst %.$(snekkja_schedule).$(if $(filter %.c,$@),c)$(if $(filter %.cpp,$@),cpp),%,$@)_graph) --platform $(call kkj_$(patsubst %.$(snekkja_schedule).$(if $(filter %.c,$@),c)$(if $(filter %.cpp,$@),cpp),%,$@)_platform) --scheduler $(call kkj_$(patsubst %.$(snekkja_schedule).$(if $(filter %.c,$@),c)$(if $(filter %.cpp,$@),cpp),%,$@)_scheduler) > $@

$(embed_object): $$(call embed_src,$$@)
	$(LD) -r -b binary -o $@ $(patsubst %.o,%,$@)

$(cc_object): $$(call source,$$@,$$(sort $$(foreach lang,cc cxx,$$(call projectof,$$@,$$(call typeof,$$@),$$(lang)))),$$(call typeof,$$@),cc)
	$(CC) -E $(GLOBAL_CFLAGS) $(call cflags_project,$(call projectof,$@,$(call typeof,$@),cc),$(call typeof,$@),cc) $(call cflags_private_object,$@) $(call cflags_file,$@,$(call typeof,$@)) $(CPPFLAGS) -o $(patsubst %.o,%.e,$@) $<
	$(CC) -c $(GLOBAL_CFLAGS) $(call cflags_project,$(call projectof,$@,$(call typeof,$@),cc),$(call typeof,$@),cc) $(call cflags_private_object,$@) $(call cflags_file,$@,$(call typeof,$@)) $(CPPFLAGS) -o $@ $<
	$(CC) -M $(GLOBAL_CFLAGS) $(call cflags_project,$(call projectof,$@,$(call typeof,$@),cc),$(call typeof,$@),cc) $(call cflags_private_object,$@) $(call cflags_file,$@,$(call typeof,$@)) $(CPPFLAGS) $< -MF $(patsubst %.o,%.d,$@)
	@# Generate dependencies to header files
	@# More tricks to manage header renaming stored in different folders (http://scottmcpeak.com/autodepend/autodepend.html)
	@mv -f $(patsubst %.o,%.d,$@) $(patsubst %o,%d,$@).tmp
	@sed -e 's|.*:|$@:|' < $(patsubst %.o,%.d,$@).tmp > $(patsubst %.o,%.d,$@)
	@sed -e 's/.*://' -e 's/\\$$//' < $(patsubst %.o,%.d,$@).tmp | fmt -1 | sed -e 's/^ *//' -e 's/$$/:/' >> $(patsubst %.o,%.d,$@)
	@rm -f $(patsubst %.o,%.d,$@).tmp

$(cxx_object): $$(call source,$$@,$$(sort $$(foreach lang,cc cxx,$$(call projectof,$$@,$$(call typeof,$$@),$$(lang)))) ,$$(call typeof,$$@),cxx)
	$(CXX) -E $(GLOBAL_CXXFLAGS) $(call cflags_project,$(call projectof,$@,$(call typeof,$@),cxx),$(call typeof,$@),cxx) $(call cflags_private_object,$@) $(call cxxflags_file,$@,$(call typeof,$@)) $(CPPFLAGS) -o $(patsubst %.o,%.e,$@) $<
	$(CXX) -c $(GLOBAL_CXXFLAGS) $(call cflags_project,$(call projectof,$@,$(call typeof,$@),cxx),$(call typeof,$@),cxx) $(call cflags_private_object,$@) $(call cxxflags_file,$@,$(call typeof,$@)) $(CPPFLAGS) -o $@ $<
	$(CXX) -M $(GLOBAL_CXXFLAGS) $(call cflags_project,$(call projectof,$@,$(call typeof,$@),cxx),$(call typeof,$@),cxx) $(call cflags_private_object,$@) $(call cxxflags_file,$@,$(call typeof,$@)) $(CPPFLAGS) $< -MF $(patsubst %.o,%.d,$@)
	@# Generate dependencies to header files
	@# More tricks to manage header renaming stored in different folders (http://scottmcpeak.com/autodepend/autodepend.html)
	@mv -f $(patsubst %.o,%.d,$@) $(patsubst %o,%d,$@).tmp
	@sed -e 's|.*:|$@:|' < $(patsubst %.o,%.d,$@).tmp > $(patsubst %.o,%.d,$@)
	@sed -e 's/.*://' -e 's/\\$$//' < $(patsubst %.o,%.d,$@).tmp | fmt -1 | sed -e 's/^ *//' -e 's/$$/:/' >> $(patsubst %.o,%.d,$@)
	@rm -f $(patsubst %.o,%.d,$@).tmp

$(bincc_list): $$(call object,$$(call suffixin,$$@,$$(bincc_package)),bin,cc) $$(call object,$$(call suffixin,$$@,$$(bincc_package)),bin,cxx,cc) $$(call $$(patsubst %,bin_%_deps,$$(call suffixin,$$@,$$(bincc_package)))) $$(call embed,$$(call suffixin,$$@,$$(bincc_package)),bin)
	$(CC) -o $@ $(if $(strip $(call bin_$@_static)),$(if $(filter 0,$(strip $(call bin_$@_static))),,--static),) $(GLOBAL_CC_LDFLAGS) $(call ldflags_project,$(call suffixin,$@,$(bincc_package),bin,cc),bin,cc) $(LDFLAGS) $(call object,$(call suffixin,$@,$(bincc_package)),bin,cc) $(call object,$(call suffixin,$@,$(bincc_package)),bin,cxx,cc) $(call embed,$(call suffixin,$@,$(bincc_package)),bin) $(call libs_project,$@,bin,cc) $(call pkg_project,$@,bin,cc) $(GLOBAL_CC_PKG) $(GLOBAL_CC_LIBS) $(LIBS)

$(bincxx_list): $$(call object,$$(call suffixin,$$@,$$(bincxx_package)),bin,cxx) $$(call object,$$(call suffixin,$$@,$$(bincxx_package)),bin,cc,cxx) $$(call $$(patsubst %,bin_%_deps,$$(call suffixin,$$@,$$(bincxx_package)))) $$(call embed,$$(call suffixin,$$@,$$(bincxx_package)),bin)
	$(CXX) -o $@ $(if $(strip $(call bin_$@_static)),$(if $(filter 0,$(strip $(call bin_$@_static))),,--static),) $(GLOBAL_CXX_LDFLAGS) $(call ldflags_project,$(call suffixin,$@,$(bincxx_package),bin,cxx),bin,cxx) $(LDFLAGS) $(call object,$(call suffixin,$@,$(bincxx_package)),bin,cxx) $(call object,$(call suffixin,$@,$(bincxx_package)),bin,cc,cxx) $(call embed,$(call suffixin,$@,$(bincxx_package)),bin) $(call libs_project,$@,bin,cxx) $(call pkg_project,$@,bin,cxx) $(GLOBAL_CXX_PKG) $(GLOBAL_CXX_LIBS) $(LIBS)

$(lib_list): $$(foreach lang,cc cxx,$$(call object,$$(patsubst lib%.a,%,$$@),lib,$$(lang)) $$(call kkj_object,$$(patsubst lib%.a,%,$$@),kkj,$$(lang)) $$(foreach type,lib kkj,$$(call $$(patsubst %,$$(type)$$(lang)_%_deps,$$(call suffixin,$$(patsubst lib%.a,%,$$@),$$($$(type)$_package)))))) $$(foreach type,lib kkj,$$(call embed,$$(patsubst lib%.a,%,$$@),$$(type),$$(lang)))
	ar rs $@ $(foreach lang,cc cxx,$(call object,$(patsubst lib%.a,%,$@),lib,$(lang)) $(call kkj_object,$(patsubst lib%.a,%,$@),kkj,$(lang))) $(call embed,$(patsubst lib%.a,%,$@),lib,) $(call embed,$(patsubst lib%.a,%,$@),kkj,)

$(testcc_list): $$(call object,$$(patsubst test_%,%,$$@),test,cc) $$(call object,$$(patsubst test_%,%,$$@),test,cxx,cc) $$(call $$(patsubst %,test_%_deps,$$(call suffixin,$$(patsubst lib%.a,%,$$@),$$(testcc_package)))) $$(call embed,$$(call suffixin,$$@,$$(testcc_package)),test)
	$(CC) -o $@ $(if $(strip $(call testcc_$(call suffixin,$@,$(testcc_package))static)),$(if $(filter 0,$(strip $(call testcc_$(call suffixin,$@,$(testcc_package))static))),,--static),) $(GLOBAL_CC_TEST_LDFLAGS) $(call ldflags_project,$(call suffixin,$@,$(testcc_package),test,cc),test,cc) $(GLOBAL_CC_TEST_LDFLAGS) $(LDFLAGS) $(call object,$(call suffixin,$@,$(testcc_package)),test,cc) $(call object,$(call suffixin,$@,$(testcc_package)),test,cxx,cc) $(call embed,$(call suffixin,$@,$(testcc_package)),test) $(call libs_project,$(call suffixin,$@,$(testcc_package)),test,cc) $(call pkg_project,$(call suffixin,$@,$(testcc_package)),test,cc) $(GLOBAL_CC_TEST_PKG) $(GLOBAL_CC_TEST_LIBS) $(LIBS)

$(testcxx_list): $$(call object,$$(patsubst test_%,%,$$@),test,cxx) $$(call object,$$(patsubst test_%,%,$$@),test,cc,cxx) $$(call $$(patsubst %,test_%_deps,$$(call suffixin,$$(patsubst lib%.a,%,$$@),$$(testcxx_package)))) $$(call embed,$$(call suffixin,$$@,$$(testcxx_package)),test)
	$(CXX) -o $@ $(if $(strip $(call testcxx_$(call suffixin,$@,$(testcxx_package))_static)),$(if $(filter 0,$(strip $(call testcxx_$(call suffixin,$@,$(testcxx_package))_static))),,--static),) $(GLOBAL_CXX_TEST_LDFLAGS) $(call ldflags_project,$(call suffixin,$@,$(testcxx_package),test,cxx),test,cxx) $(LDFLAGS) $(call object,$(call suffixin,$@,$(testcxx_package)),test,cxx) $(call object,$(call suffixin,$@,$(testcxx_package)),test,cc,cxx) $(call embed,$(call suffixin,$@,$(testcxx_package)),test) $(call libs_project,$(call suffixin,$@,$(testcxx_package)),test,cxx) $(call pkg_project,$(call suffixin,$@,$(testcxx_package)),test,cxx) $(GLOBAL_CXX_TEST_PKG) $(GLOBAL_CXX_TEST_LIBS) $(LIBS)

$(sharedcc_list): $$(call object,$$(patsubst lib%.so.$$(version).$$(minor).$$(release),%,$$@),shared,cc) $$(call object,$$(patsubst lib%.so.$$(version).$$(minor).$$(release),%,$$@),shared,cxx,cc) $$(call $$(patsubst %,shared_%_deps,$$(call suffixin,$$(patsubst lib%.so.$$(version).$$(minor).$$(release),%,$$@),$$(sharedcc_package)))) $$(call embed,$$(call suffixin,$$@,$$(sharedcc_package)),shared)
	$(CC) -shared -Wl,-soname=$(patsubst %.$(minor).$(release),%,$@) -o $@ $(if $(strip $(call sharedcc_$(call suffixin,$(patsubst lib%.so.$(version).$(minor).$(release),%,$@),$(sharedcc_package))static)),$(if $(filter 0,$(strip $(call sharedcc_$(call suffixin,$(patsubst lib%.so.$(version).$(minor).$(release),%,$@),$(sharedcc_package))static))),,--static),) $(GLOBAL_CC_SHARED_LDFLAGS) $(call ldflags_project,$(call suffixin,$(patsubst lib%.so.$(version).$(minor).$(release),%,$@),$(sharedcc_package),shared,cc),shared,cc) $(GLOBAL_CC_SHARED_LDFLAGS) $(LDFLAGS) $(sort $(call object,$(call suffixin,$(patsubst lib%.so.$(version).$(minor).$(release),%,$@),$(sharedcc_package)),shared,cc) $(call object,$(call suffixin,$(patsubst lib%.so.$(version).$(minor).$(release),%,$@),$(sharedcc_package)),shared,cxx,cc) $(call embed,$(call suffixin,$(patsubst lib%.so.$(version).$(minor).$(release),%,$@),$(sharedcc_package)),shared)) $(call libs_project,$(call suffixin,$(patsubst lib%.so.$(version).$(minor).$(release),%,$@),$(sharedcc_package)),shared,cc) $(call pkg_project,$(call suffixin,$(patsubst lib%.so.$(version).$(minor).$(release),%,$@),$(sharedcc_package)),shared,cc) $(GLOBAL_CC_SHARED_PKG) $(GLOBAL_CC_SHARED_LIBS) $(LIBS)

$(sharedcxx_list): $$(call object,$$(patsubst lib%.so.$$(version).$$(minor).$$(release),%,$$@),shared,cxx) $$(call object,$$(patsubst lib%.so.$$(version).$$(minor).$$(release),%,$$@),shared,cc,cxx) $$(call $$(patsubst %,shared_%_deps,$$(call suffixin,$$(patsubst lib%.so.$$(version).$$(minor).$$(release),%,$$@),$$(sharedcxx_package)))) $$(call embed,$$(call suffixin,$$@,$$(sharedcxx_package)),shared)
	$(CXX) -shared -Wl,-soname=$(patsubst %.$(minor).$(release),%,$@) -o $@ $(if $(strip $(call shared_$(call suffixin,$(patsubst lib%.so.$(version).$(minor).$(release),%,$@),$(sharedcxx_package))_static)),$(if $(filter 0,$(strip $(call shared_$(call suffixin,$(patsubst lib%.so.$(version).$(minor).$(release),%,$@),$(sharedcxx_package))_static))),,--static),) $(GLOBAL_CXX_SHARED_LDFLAGS) $(call ldflags_project,$(call suffixin,$(patsubst lib%.so.$(version).$(minor).$(release),%,$@),$(sharedcxx_package),shared,cxx),shared,cxx) $(LDFLAGS) $(sort $(call object,$(call suffixin,$(patsubst lib%.so.$(version).$(minor).$(release),%,$@),$(sharedcxx_package)),shared,cxx) $(call object,$(call suffixin,$(patsubst lib%.so.$(version).$(minor).$(release),%,$@),$(sharedcxx_package)),shared,cc,cxx) $(call embed,$(call suffixin,$(patsubst lib%.so.$(version).$(minor).$(release),%,$@),$(sharedcxx_package)),shared)) $(call libs_project,$(call suffixin,$(patsubst lib%.so.$(version).$(minor).$(release),%,$@),$(sharedcxx_package)),shared,cxx) $(call pkg_project,$(call suffixin,$(patsubst lib%.so.$(version).$(minor).$(release),%,$@),$(sharedcxx_package)),shared,cxx) $(GLOBAL_CXX_SHARED_PKG) $(GLOBAL_CXX_SHARED_LIBS) $(LIBS)

$(shared_linkername): $$@.$(version)
	ln -sf $(patsubst %,%.$(version),$@) $@

$(shared_soname): $$@.$(minor).$(release)
	ln -sf $(patsubst %,%.$(minor).$(release),$@) $@

$(lib_pkgconfig): $$(patsubst %,lib%.a,$$(filter $$(patsubst %.pc,%,$$@),$(lib_package))) $$(if $$(strip $$(if $(sharedcxx_package),$$(filter $$(patsubst %.pc,%,$$@),$(sharedcxx_package))) $$(if $(sharedcc_package),$$(filter $$(patsubst %.pc,%,$$@)),$(sharedcc_package))),$$(patsubst %.pc,lib%.so,$$@))
	echo prefix=$(final_prefix) > $@
	echo exec_prefix=$(if $(foreach var,lib shared,$(call $(var)_$(patsubst %.pc,%,$@)_pkg_exec_prefix)),$(sort $(foreach var,lib shared,$(call $(var)_$(patsubst %.pc,%,$@)_pkg_exec_prefix))),'$$'\{prefix\}/bin) >> $@
	echo libdir=$(if $(sort $(foreach var,lib shared,$(call $(var)_$(patsubst %.pc,%,$@)_pkg_libdir))),$(sort $(foreach var,lib shared,$(call $(var)_$(patsubst %.pc,%,$@)_pkg_libdir))),'$$'\{prefix\}/lib) >> $@
	echo includedir=$(if $(sort $(foreach var,lib shared,$(call $(var)_$(patsubst %.pc,%,$@)_pkg_includedir))),$(sort $(foreach var,lib shared,$(call $(var)_$(patsubst %.pc,%,$@)_pkg_includedir))),'$$'\{prefix\}/include) >> $@
	echo >> $@
	echo Name: $(if $(sort $(foreach var,lib shared,$(call $(var)_$(patsubst %.pc,%,$@)_pkg_name))),$(sort $(foreach var,lib shared,$(call $(var)_$(patsubst %.pc,%,$@)_pkg_name))),$(patsubst %.pc,lib%,$@)) >> $@
	echo Description: $(sort $(foreach var,lib shared,$(call $(var)_$(patsubst %.pc,%,$@)_pkg_description))) >> $@
	echo Version: $(version).$(minor).$(release) >> $@
	echo URL: $(sort $(foreach var,lib shared,$(call $(var)_$(patsubst %.pc,%,$@)_pkg_url))) >> $@
	echo Libs: -L'$$'\{libdir\} -l$(patsubst %.pc,%,$@) $(sort $(foreach var,lib shared,$(call $(var)_$(patsubst %.pc,%,$@)_pkg_libs))) >> $@
	echo Libs.private: $(GLOBAL_LDFLAGS) $(call $2_$1_ldflags) $(GLOBAL_LIBS) $(sort $(foreach var,lib shared,$(call $(var)_$(patsubst %.pc,%,$@)_libs))) $(sort $(foreach var,lib shared,$(call $(var)_$(patsubst %.pc,%,$@)_pkg_libs_private))) >> $@
	echo Requires: $(sort $(foreach var,lib shared,$(call $(var)_$(patsubst %.pc,%,$@)_pkg_require))) >> $@
	echo Requires.private: $(foreach var,$(PKG),"$(var) $(call PKG_$(var)_version)") $(foreach var,$(call $(var)_$(patsubst %.pc,%,$@)_pkg),"$(var) $(sort $(foreach var2,lib shared,$(call $(var2)_$(patsubst %.pc,%,$@)_pkg_$(var)_version)))") $(sort $(foreach var,lib shared,$(call $(var)_$(patsubst %.pc,%,$@)_pkg_require_private))) >> $@
	echo Conflicts: $(sort $(foreach var,lib shared,$(call $(var)_$(patsubst %.pc,%,$@)_pkg_conflicts))) >> $@
	echo Cflags: $(GLOBAL_CFLAGS_EXPORT) $() -I'$$'\{includedir\} $(foreach var,cc cxx,$(call cflags_project_export,$(patsubst %.pc,%,$@),lib,$(var))) $(CPPFLAGS) $(foreach var,cc cxx,$(sort $(foreach var,lib shared,$(call $(var2)_$(patsubst %.pc,%,$@)_pkg_$(call prefix_cflags,$(var))flags)))) >> $@

dist: submake
	mkdir -p $(distdir)
	cp Makefile $(distdir)
	cp Makefile.in $(distdir)
	$(if $(strip $(bincc_package:%=bin_%_export)),cp $(sort $(foreach var,$(bincc_package:%=bin_%_export),$(call $(var)))) $(distdir),)
	$(if $(strip $(bincxx_package:%=bin_%_export)),cp $(sort $(foreach var,$(bincxx_package:%=bin_%_export),$(call $(var)))) $(distdir),)
	$(if $(strip $(lib_package:%=lib_%_export)),cp $(sort $(foreach var,$(lib_package:%=lib_%_export),$(call $(var)))) $(distdir),)
	$(if $(strip $(testcc_package:%=test_%_export)),cp $(sort $(foreach var,$(testcc_package:%=test_%_export),$(call $(var)))) $(distdir),)
	$(if $(strip $(testcxx_package:%=test_%_export)),cp $(sort $(foreach var,$(testcxx_package:%=test_%_export),$(call $(var)))) $(distdir),)
	$(if $(strip $(sharedcc_package:%=shared_%_export)),cp $(sort $(foreach var,$(sharedcc_package:%=shared_%_export),$(call $(var)))) $(distdir),)
	$(if $(strip $(sharedcxx_package:%=shared_%_export)),cp $(sort $(foreach var,$(sharedcxx_package:%=shared_%_export),$(call $(var)))) $(distdir),)
        
clean: submake
	$(RM) $(cc_object) $(foreach var,$(cc_object),$(var:%.o=%.d)) $(foreach var,$(cc_object),$(var:%.o=%.e))
	$(RM) $(cxx_object) $(foreach var,$(cxx_object),$(var:%.o=%.d)) $(foreach var,$(cxx_object),$(var:%.o=%.e))
	$(RM) $(lib_pkgconfig)
	$(RM) $(snekkja_schedule_src)
	$(RM) $(embed_object)
	$(RM) $(test_target)
	$(RM) $(bin_list)
	$(RM) $(lib_list)
	$(RM) $(shared_list) $(shared_linkername) $(shared_soname)
	$(RM) $(clean)

check: submake $(test_list)
	@$(shell echo for i in "$(foreach var,$(testcc_list),$(var))"\; do eval echo Running \$$i...\; eval ./\$$i\; done)
	@$(shell echo for i in "$(foreach var,$(testcxx_list),$(var))"\; do eval echo Running \$$i...\; eval ./\$$i\; done)

checkdist distcheck: dist
        
install: submake $(bin_list) $(lib_list) $(shared_list) $(shared_linkername) $(shared_soname) $(lib_pkgconfig)
	$(if $(strip $(lib_list)),install -d $(DESTDIR)$(final_libdir))
	$(if $(strip $(lib_list)),install -m 644 $(lib_list) $(DESTDIR)$(final_libdir))
	$(if $(strip $(shared_list)),install -d $(DESTDIR)$(final_libdir))
	$(if $(strip $(shared_list)),install -m 755 $(shared_list) $(DESTDIR)$(final_libdir))
	$(if $(strip $(shared_linkername)),cp -a $(shared_linkername) $(DESTDIR)$(final_libdir))
	$(if $(strip $(shared_soname)),cp -a $(shared_soname) $(DESTDIR)$(final_libdir))
	$(if $(strip $(bin_list)),install -d $(DESTDIR)$(final_bindir))
	$(if $(strip $(bin_list)),install -m 755 $(bin_list) $(DESTDIR)$(final_bindir))
	$(if $(strip $(lib_list)),install -d $(DESTDIR)$(final_libdir)/pkgconfig)
	$(if $(strip $(shared_list)),install -d $(DESTDIR)$(final_libdir)/pkgconfig)
	$(if $(strip $(lib_pkgconfig)),install -m 644 $(lib_pkgconfig) $(DESTDIR)$(final_libdir)/pkgconfig)

uninstall: submake
	$(RM) $(foreach var,$(bin_list), $(DESTDIR)$(final_bindir)/$(var))
	$(RM) $(foreach var,$(lib_list), $(DESTDIR)$(final_libdir)/$(var))
	$(RM) $(foreach var,$(shared_list), $(DESTDIR)$(final_libdir)/$(var))
	$(RM) $(foreach var,$(shared_linkername), $(DESTDIR)$(final_libdir)/$(var))
	$(RM) $(foreach var,$(shared_soname), $(DESTDIR)$(final_libdir)/$(var))
	$(RM) $(foreach var,$(lib_pkgconfig), $(DESTDIR)$(final_libdir)/pkgconfig/$(var))
	
submake:
	@$(shell echo for i in "$(foreach var,$(subdirs),$(var))"\; do $(MAKE) -C \$$i $(MAKECMDGOALS) distdir=$(distdir)/\$$i subdir=$(subdir)/\$$i\; done)

FORCE:
.PHONY: all clean check install uninstall dist FORCE submake
