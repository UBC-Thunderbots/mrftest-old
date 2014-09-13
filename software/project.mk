#
# The executables to compile.
#
EXECUTABLES := ai convertlog cppunit getcore hall2phase log mrfcap mrftest sdutil autoref

#
# The subset of the above that should not be built by the world target.
#
EXECUTABLES_EXCLUDE_WORLD := cppunit

#
# The source files for each executable.
# Directories will be searched recursively for source files.
#
SOURCES_ai := ai drive geom log/shared main.cpp mrf proto uicomponents util
SOURCES_convertlog := ai/common convertlog log/shared main.cpp proto util
SOURCES_cppunit := cppunit geom util
SOURCES_getcore := getcore.cpp main.cpp util
SOURCES_hall2phase := hall2phase
SOURCES_log := ai/common geom log main.cpp proto uicomponents util
SOURCES_mrfcap := main.cpp mrfcap.cpp util
SOURCES_mrftest := drive main.cpp mrf test/common test/mrf uicomponents util
SOURCES_sdutil := main.cpp sdutil.cpp util
SOURCES_autoref :=  autoref main.cpp util  geom ai/backend proto ai/common ai/ai.h ai/ai.cpp ai/param_panel.h ai/param_panel.cpp ai/util.cpp ai/util.h  mrf drive ai/robot_controller ai/window.h ai/window.cpp uicomponents

#
# All the pkg-config packages used.
#
PACKAGES := glibmm-2.4 gtkmm-2.4 cairomm-1.0 ode protobuf libxml++-2.6 gsl libusb-1.0

#
# The flags to pass to the linker ahead of any object files.
#
PROJECT_LDFLAGS := -pthread -Wl,--as-needed -Wl,-O1 -g

#
# The library flags to pass to the linker after all object files.
#
PROJECT_LIBS := -lbz2

#
# The flags to pass to the C++ compiler.
#
PROJECT_CXXFLAGS := -std=gnu++11 -pthread -Wall -Wextra -Wformat=2 -Wstrict-aliasing=2 -Wold-style-cast -Wconversion -Wundef -Wmissing-declarations -Wredundant-decls -march=native -O2 -fomit-frame-pointer -fstrict-aliasing -g -D_FILE_OFFSET_BITS=64 -D__STDC_CONSTANT_MACROS=1 -DHAVE_INLINE -I.

#
# Custom pkg-config packages to use only for the cppunit target.
# This will only apply during linking, because a single compilation step can be shared between multiple executable targets.
#
PACKAGES_cppunit := cppunit

#
# Runs the unit tests.
#
.PHONY : check
check : bin/cppunit
	bin/cppunit
