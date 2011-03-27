# ========== STOP ========== STOP ========== STOP ==========
#
# The contents of this file should not generally be modified.
# The rules in this file are generic and compatible with building multiple executables.
# To change the set of executables compiled, the set sources for each executable, or the compilation or linking flags, edit project.mk instead.
#
# ========== STOP ========== STOP ========== STOP ==========

#
# Some rules will need second expansions as they are static pattern rules which do variable dereferences based on target name.
#
.SECONDEXPANSION :

#
# Pull in the project rules.
#
include project.mk

#
# Build a full CXXFLAGS, LDFLAGS, and LIBS.
#
override CXXFLAGS := $(PROJECT_CXXFLAGS) $(shell pkg-config --cflags $(PACKAGES) | sed 's/-I/-isystem /g') $(CXXFLAGS)
override LDFLAGS := $(PROJECT_LDFLAGS) $(shell pkg-config --libs-only-other --libs-only-L $(PACKAGES)) $(LDFLAGS)
override LIBS := $(PROJECT_LIBS) $(shell pkg-config --libs-only-l $(PACKAGES)) $(LIBS)

#
# The target to build everything.
#
.PHONY : world
world : $(addprefix bin/,$(EXECUTABLES))

#
# Prevent echoing of compilation commands.
#
.SILENT :

#
# Rule to build the documentation.
#
.PHONY : doc
doc :
	doxygen

#
# Rule to build a cross-reference database.
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
# Rule to link a final executable.
#
$(addprefix bin/,$(EXECUTABLES)) : bin/% : $$(addprefix obj/,$$(subst .cc,.o,$$(subst .cpp,.o,$$(shell find $$(SOURCES_%) -name .svn -prune -o -name '*.cpp' -print) $$(if $$(filter proto,$$(SOURCES_%)),$(PROTOBUF_CCS)))))
	echo "  LD	$@"
	mkdir -p bin
	$(CXX) $(LDFLAGS) -o$@ $+ $(LIBS)

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
	$(CXX) $(CXXFLAGS) -o$@ -c $<

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
ALL_D_FILES=$(addprefix obj/,$(subst ./,,$(subst .cpp,.d,$(shell find . -name .svn -prune -o -name '*.cpp' -print))))
ifneq ($(MAKECMDGOALS),clean)
ifneq ($(MAKECMDGOALS),cleanbin)
ifneq ($(MAKECMDGOALS),cleandep)
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
	find . \( -name .svn -prune \) -o \( -path ./proto -prune \) -o \( \( -name '*.cpp' -o -name '*.h' \) -exec uncrustify -c uncrustify.cfg --no-backup \{\} \+ \)

#
# Rules to clean.
#
.PHONY : clean
clean : cleanbin cleandoc cleanobj cleanpb cleantags

.PHONY : cleanbin
cleanbin :
	$(RM) -rf bin

.PHONY : cleandep
cleandep :
	find obj -name '*.d' -delete

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
