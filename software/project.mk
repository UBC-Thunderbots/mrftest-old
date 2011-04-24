#
# The executables to compile.
#
EXECUTABLES := ai fw log simulator test xbeeconfig

#
# The source files for each executable.
# Directories will be searched recursively for source files.
#
SOURCES_ai := ai geom log/shared proto uicomponents util xbee
SOURCES_fw := fw geom util xbee
SOURCES_log := ai/common log geom proto uicomponents util
SOURCES_simulator := ai/common/playtype.cpp geom simulator util
SOURCES_test := geom test uicomponents util xbee
SOURCES_xbeeconfig := util xbee xbeeconfig.cpp

#
# All the pkg-config packages used.
#
PACKAGES := glibmm-2.4 gtkmm-2.4 cairomm-1.0 ode protobuf libxml++-2.6 gsl libusb-1.0

#
# The flags to pass to the linker ahead of any object files.
#
PROJECT_LDFLAGS := -Wl,--as-needed -Wl,-O1 -g

#
# The library flags to pass to the linker after all object files.
#
PROJECT_LIBS := -lrt

#
# The flags to pass to the C++ compiler.
#
PROJECT_CXXFLAGS := -std=gnu++0x -Wall -Wextra -Wold-style-cast -Wconversion -Wundef -march=native -O2 -fomit-frame-pointer -g -D_FILE_OFFSET_BITS=64 -D__STDC_CONSTANT_MACROS=1 -DHAVE_INLINE -I.
