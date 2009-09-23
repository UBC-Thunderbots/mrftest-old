#
# Here, write a list of all the executable binaries the Makefile might generate.
#
PROJECTS := rctest thunderbots xbeeconfig

#
# Here, write a list of the source files and directories for each executable.
# Directories will be searched for source files recursively.
#
SOURCES_rctest := geom robot_controller robot_controller_test util world
SOURCES_thunderbots := ai geom robot_controller simulator uicomponents util world main.cpp
SOURCES_xbeeconfig := xbeeconfig.cpp

#
# Here, write the linker flags to be used for each project.
#
LDFLAGS_common := $(shell pkg-config --libs glibmm-2.4 gtkmm-2.4 cairomm-1.0 libxml++-2.6) -lrt
LDFLAGS_rctest := $(LDFLAGS_common)
LDFLAGS_thunderbots := $(LDFLAGS_common)
LDFLAGS_xbeeconfig := $(LDFLAGS_common) -lrt

#
# Here, write the compiler flags to be used.
#
CXXFLAGS := -Wall -Wextra -O2 -fomit-frame-pointer -pthread $(shell pkg-config --cflags glibmm-2.4 gtkmm-2.4 cairomm-1.0 libxml++-2.6) $(CXXFLAGS) -I.

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
# Template of code to generate for each project.
#
define project_template
#
# Find all the cpp sources for the project.
#
CPP_SOURCES_EXPANDED_$(1) := $$(shell find $$(SOURCES_$(1)) -name .svn -prune -o -name '*.cpp' -print)
ALL_CPP_FILES += $$(CPP_SOURCES_EXPANDED_$(1))

$(1) : $$(CPP_SOURCES_EXPANDED_$(1):.cpp=.o)
	$$(CXX) $$(CXXFLAGS) -o$$@ $$+ $$(LDFLAGS_$(1))
endef

#
# Generate the code for the projects.
#
$(eval $(foreach proj,$(PROJECTS),$(call project_template,$(proj))))

#
# Rule to make a .d file from a .cpp file.
#
%.d : %.cpp
	$(CXX) $(CXXFLAGS) -MT '$(@:.d=.o) $@' -MM -MF $@ $< || { $(RM) $@; false; }

#
# Include all the dependency files.
#
ifneq ($(MAKECMDGOALS),clean)
include $(ALL_CPP_FILES:.cpp=.d)
endif

#
# Rule to clean.
#
.PHONY : clean
clean :
	find . \( -name '*.o' -o -name '*.d' \) -execdir rm \{\} +
	$(RM) $(PROJECTS)
