#
# Here, write a list of all the executable binaries the Makefile might generate.
#
PROJECTS := ai config fw log simulator test

#
# Here, write a list of the source files and directories for each executable.
# Directories will be searched for source files recursively.
#
SOURCES_ai := ai geom leastsquares log/shared proto uicomponents util xbee
SOURCES_config := config geom leastsquares uicomponents util
SOURCES_fw := fw geom leastsquares util xbee
SOURCES_log := ai/common leastsquares log geom proto uicomponents util
SOURCES_simulator := ai/common/playtype.cpp geom leastsquares simulator util
SOURCES_test := geom leastsquares test uicomponents util xbee

#
# Here, write a list of all the pkg-config packages used.
#
PACKAGES := glibmm-2.4 gtkmm-2.4 cairomm-1.0 ode protobuf libxml++-2.6 gsl libusb-1.0

#
# Here, write the linker flags to be used for each project.
#
override LDFLAGS := -Wl,--as-needed -Wl,-O1 -g -pthread $(LDFLAGS)
override LIBS := $(shell pkg-config --libs $(PACKAGES)) -lrt $(LIBS)

#
# Here, write the compiler flags to be used.
#
override CXXFLAGS := -std=gnu++0x -Wall -Wextra -Wold-style-cast -Wconversion -Wundef -O2 -fomit-frame-pointer -g -D_FILE_OFFSET_BITS=64 -D__STDC_CONSTANT_MACROS=1 -pthread $(shell pkg-config --cflags $(PACKAGES) | sed 's/-I/-isystem /g') $(CXXFLAGS) -I.

#
# DO NOT MODIFY STUFF BELOW HERE WITHOUT A GOOD REASON.
# It should all be automated.
#

#
# The target to build everything.
#
.PHONY : world
world : $(addprefix bin/,$(PROJECTS))

#
# Prevent echoing of compilation commands.
#
.SILENT :

#
# Build the documentation.
#
.PHONY : doc
doc :
	doxygen

#
# Build a cross-reference database.
#
.PHONY : tags
tags :
	ctags -R

#
# Gather all the protocol buffers input files.
#
PROTOBUF_INPUTS := $(wildcard proto/*.proto)
PROTOBUF_CCS := $(PROTOBUF_INPUTS:.proto=.pb.cc)
PROTOBUF_HS := $(PROTOBUF_INPUTS:.proto=.pb.h)

#
# Template of code to generate for each project.
#
define project_template
#
# Find all the cpp sources for the project.
#
CPP_SOURCES_EXPANDED_$(1) := $$(shell find $$(SOURCES_$(1)) -name .svn -prune -o -name '*.cpp' -print)
ALL_CPP_FILES += $$(CPP_SOURCES_EXPANDED_$(1))
ifneq (,$$(filter proto,$$(SOURCES_$(1))))
CC_SOURCES_EXPANDED_$(1) := $(PROTOBUF_CCS)
else
CC_SOURCES_EXPANDED_$(1) :=
endif

OBJS_$(1) := $$(addprefix obj/,$$(CPP_SOURCES_EXPANDED_$(1):.cpp=.o) $$(CC_SOURCES_EXPANDED_$(1):.cc=.o))

bin/$(1) : $$(OBJS_$(1))
	echo "  LD	$$@"
	mkdir -p bin
	$$(CXX) $(LDFLAGS) -o$$@ $$+ $(LIBS)
endef

#
# Generate the code for the projects.
#
$(eval $(foreach proj,$(PROJECTS),$(call project_template,$(proj))))

#
# Rule to make a .d file from a .cpp file.
#
# Protobuf interferes with normal automatic dependency calculation, because the
# C++ header files corresponding to protobuf files do not exist when the
# dependency-calculating rule is invoked, thus causing the C++ compiler to throw
# an error on the #include line while trying to compute the dependencies. To
# solve this, just make all ".d" files depend on the protobuf ".h" files, to
# force protoc to be run before dependency calculation begins.
#
obj/%.d : %.cpp $(PROTOBUF_HS)
	echo "  DEP	$@"
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MT '$(@:.d=.o) $@' -MM -MF $@ $< || { $(RM) $@; false; }

#
# Rule to make a .o file from a .cpp file.
#
obj/%.o : %.cpp
	echo "  CXX	$@"
	mkdir -p $(dir $@)
	$(CXX) $(if $(filter obj/leastsquares/%,$@),$(filter-out -Wconversion,$(CXXFLAGS)),$(CXXFLAGS)) -o$@ -c $<

#
# Rule to make a .pb.cc and a .pb.h file from a .proto file.
#
proto/%.pb.cc proto/%.pb.h : proto/%.proto
	echo "  PROTO	$@"
	protoc --proto_path=proto --cpp_out=proto $<
.SECONDARY : $(PROTOBUF_CCS) $(PROTOBUF_HS)

#
# Customized rule to compile a protocol-buffers-generated C++ source file,
# because the generated code otherwise triggers warnings.
#
obj/proto/%.pb.o : proto/%.pb.cc $(PROTOBUF_HS)
	echo "  CXX	$@"
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -o$@ -c -Wno-extra $<

#
# Include all the dependency files.
#
ALL_D_FILES=$(addprefix obj/,$(ALL_CPP_FILES:.cpp=.d))
ifneq ($(MAKECMDGOALS),clean)
ifneq ($(MAKECMDGOALS),cleanbin)
ifneq ($(MAKECMDGOALS),cleandoc)
ifneq ($(MAKECMDGOALS),cleanobj)
ifneq ($(MAKECMDGOALS),cleanpb)
ifneq ($(MAKECMDGOALS),cleantags)
ifneq ($(MAKECMDGOALS),tags)
ifneq ($(MAKECMDGOALS),doc)
ifneq ($(MAKECMDGOALS),uncrustify)
include $(ALL_D_FILES)
endif
endif
endif
endif
endif
endif
endif
endif
endif

#
# Rule to make only the dependency files.
#
.PHONY : dep
dep : $(ALL_D_FILES)

#
# Rule to clean and format source with uncrustify.
#
.PHONY : uncrustify
uncrustify :
	find . \( -name .svn -prune \) -o \( -path ./leastsquares -prune \) -o \( -path ./proto -prune \) -o \( \( -name '*.cpp' -o -name '*.h' \) -exec uncrustify -c uncrustify.cfg --no-backup \{\} \+ \)

#
# Rules to clean.
#
.PHONY : clean
clean : cleanbin cleandoc cleanobj cleanpb cleantags

.PHONY : cleanbin
cleanbin :
	$(RM) -rf bin

.PHONY : cleandoc
cleandoc :
	$(RM) -rf html

.PHONY : cleanobj
cleanobj :
	$(RM) -rf obj

.PHONY : cleanpb
cleanpb :
	$(RM) -f $(PROTOBUF_CCS) $(PROTOBUF_HS)

.PHONY : cleantags
cleantags :
	$(RM) -f tags
