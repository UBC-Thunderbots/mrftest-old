#
# The executables to compile.
#
EXECUTABLES := ai convertlog cppunit crc16 experiment fw hall2phase log mrfcap mrfdbg mrftest simulator test xbeeconfig

#
# The subset of the above that should not be built by the world target.
#
EXECUTABLES_EXCLUDE_WORLD := cppunit

#
# The source files for each executable.
# Directories will be searched recursively for source files.
#
SOURCES_ai := ai geom log/shared mrf proto uicomponents util xbee
SOURCES_convertlog := ai/common convertlog geom log/shared proto util
SOURCES_cppunit := cppunit geom util
SOURCES_crc16 := crc16.cpp geom util
SOURCES_experiment := experiment geom util xbee
SOURCES_fw := fw geom util xbee
SOURCES_hall2phase := hall2phase
SOURCES_log := ai/common log geom proto uicomponents util
SOURCES_mrfcap := geom mrfcap util
SOURCES_mrfdbg := geom mrfdbg util
SOURCES_mrftest := geom mrf mrftest uicomponents util
SOURCES_simulator := ai/common/playtype.cpp geom simulator util
SOURCES_test := geom test uicomponents util xbee
SOURCES_xbeeconfig := geom util xbee xbeeconfig.cpp

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
PROJECT_LIBS := -lbz2 -lrt

#
# The flags to pass to the C++ compiler.
#
PROJECT_CXXFLAGS := -std=gnu++0x -pthread -Wall -Wextra -Wold-style-cast -Wconversion -Wundef -march=native -O2 -fomit-frame-pointer -g -D_FILE_OFFSET_BITS=64 -D__STDC_CONSTANT_MACROS=1 -DHAVE_INLINE -I.

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
