#
# Here, write a list of all the executable binaries the Makefile might generate.
#
PROJECTS := config fw rxdata simulator test thunderbots xbeeconfig xbeed xbeerconfig

#
# Here, write a list of the source files and directories for each executable.
# Directories will be searched for source files recursively.
#
SOURCES_config := configeditor uicomponents util xbee/client
SOURCES_fw := firmware uicomponents util xbee/client xbee/shared
SOURCES_rxdata := rxdata.cpp uicomponents util xbee/client xbee/shared
SOURCES_simulator := geom leastsquares proto sim uicomponents util xbee/client xbee/daemon/frontend xbee/shared
SOURCES_steptest := geom leastsquares util xbee steptest.cpp
SOURCES_test := geom tester uicomponents util xbee/client xbee/shared
SOURCES_thunderbots := ai geom leastsquares proto robot_controller uicomponents util xbee/client xbee/shared
SOURCES_xbeeconfig := uicomponents util xbeeconfig.cpp xbee/client
SOURCES_xbeed := uicomponents util xbee/client xbee/daemon xbee/shared
SOURCES_xbeerconfig := uicomponents util xbeerconfig.cpp xbee/client xbee/shared

#
# Here, write the linker flags to be used for each project.
#
LDFLAGS_common := $(shell pkg-config --libs glibmm-2.4 gtkmm-2.4 cairomm-1.0 ode protobuf) -lrt
LDFLAGS_config := $(LDFLAGS_common)
LDFLAGS_fw := $(LDFLAGS_common)
LDFLAGS_rxdata := $(LDFLAGS_common)
LDFLAGS_simulator := $(LDFLAGS_common)
LDFLAGS_steptest := $(LDFLAGS_common)
LDFLAGS_test := $(LDFLAGS_common)
LDFLAGS_thunderbots := $(LDFLAGS_common)
LDFLAGS_xbeeconfig := $(LDFLAGS_common)
LDFLAGS_xbeed := $(LDFLAGS_common)
LDFLAGS_xbeerconfig := $(LDFLAGS_common)

#
# Here, write the compiler flags to be used.
#
override CXXFLAGS := -std=gnu++0x -Wall -Wextra -Wold-style-cast -O2 -fomit-frame-pointer -g -D_FILE_OFFSET_BITS=64 -D__STDC_CONSTANT_MACROS=1 -pthread $(shell pkg-config --cflags glibmm-2.4 gtkmm-2.4 cairomm-1.0 ode protobuf | sed 's/-I/-isystem /g') $(CXXFLAGS) -I.

#
# DO NOT MODIFY STUFF BELOW HERE WITHOUT A GOOD REASON.
# It should all be automated.
#

#
# The target to build everything.
#
.PHONY : world
world : $(PROJECTS)

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
# Template of code to generate for each project.
#
define project_template
#
# Find all the cpp sources for the project.
#
CPP_SOURCES_EXPANDED_$(1) := $$(shell find $$(SOURCES_$(1)) -name .svn -prune -o -name '*.cpp' -print)
ALL_CPP_FILES += $$(CPP_SOURCES_EXPANDED_$(1))
CC_SOURCES_EXPANDED_$(1) := $$(shell find $$(SOURCES_$(1)) -name .svn -prune -o -name '*.cc' -print)

$(1) : $$(CPP_SOURCES_EXPANDED_$(1):.cpp=.o) $$(CC_SOURCES_EXPANDED_$(1):.cc=.o)
	echo "  LD $$@"
	$$(CXX) $$(CXXFLAGS) -o$$@ $$+ $$(LDFLAGS_$(1))
endef

#
# Generate the code for the projects.
#
$(eval $(foreach proj,$(PROJECTS),$(call project_template,$(proj))))

#
# Gather all the protocol buffers input files.
#
PROTOBUF_INPUTS := $(wildcard proto/*.proto)
PROTOBUF_CCS := $(PROTOBUF_INPUTS:.proto=.pb.cc)
PROTOBUF_HS := $(PROTOBUF_INPUTS:.proto=.pb.h)

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
%.d : %.cpp $(PROTOBUF_HS)
	echo "  DEP $@"
	$(CXX) $(CXXFLAGS) -MT '$(@:.d=.o) $@' -MM -MF $@ $< || { $(RM) $@; false; }

#
# Rule to make a .o file from a .cpp file.
#
%.o : %.cpp
	echo "  CXX $@"
	$(CXX) $(CXXFLAGS) -o$@ -c $<

#
# Rule to make a .pb.cc and a .pb.h file from a .proto file.
#
proto/%.pb.cc proto/%.pb.h : proto/%.proto
	echo "  PROTOC $@"
	protoc --proto_path=proto --cpp_out=proto $<
.SECONDARY : $(PROTOBUF_CCS) $(PROTOBUF_HS)

#
# Customized rule to compile a protocol-buffers-generated C++ source file,
# because the generated code otherwise triggers warnings.
#
proto/%.pb.o : proto/%.pb.cc $(PROTOBUF_HS)
	echo "  CXX $@"
	$(CXX) $(CXXFLAGS) -o$@ -c -Wno-extra $<

#
# Include all the dependency files.
#
ifneq ($(MAKECMDGOALS),clean)
ifneq ($(MAKECMDGOALS),cleanbin)
ifneq ($(MAKECMDGOALS),cleandep)
ifneq ($(MAKECMDGOALS),cleanobj)
ifneq ($(MAKECMDGOALS),cleanpb)
include $(ALL_CPP_FILES:.cpp=.d)
endif
endif
endif
endif
endif

#
# Rule to make only the dependency files.
#
.PHONY : dep
dep : $(ALL_CPP_FILES:.cpp=.d)

#
# Rules to clean.
#
.PHONY : clean
clean : cleanbin cleandep cleanobj cleanpb

.PHONY : cleanbin
cleanbin :
	$(RM) $(PROJECTS)

.PHONY : cleandep
cleandep :
	find . ! -path '*/.svn*' -name '*.d' -delete

.PHONY : cleanobj
cleanobj :
	find . ! -path '*/.svn*' -name '*.o' -delete

.PHONY : cleanpb
cleanpb :
	$(RM) $(PROTOBUF_CCS) $(PROTOBUF_HS)
