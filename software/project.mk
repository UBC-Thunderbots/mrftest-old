#
# The executables to compile.
#
EXECUTABLES := ai buildid test getcore hall2phase log mrfcap mrftest nulltest sdutil

#
# The subset of the above that should not be built by the world target.
#
EXECUTABLES_EXCLUDE_WORLD := test

#
# The source files for each executable.
# Directories will be searched recursively for source files.
#
SOURCES_ai := ai drive geom log/shared main.cpp mrf proto uicomponents util
SOURCES_buildid := build_id.cpp main.cpp util
SOURCES_test := test/unit-tests geom util/param.* util/string.* util/config.* util/exception.* util/misc.* util/dprint.* util/hungarian.* util/matrix.* util/codec.*
SOURCES_getcore := getcore.cpp main.cpp util
SOURCES_hall2phase := hall2phase
SOURCES_log := ai/common/playtype.* ai/common/colour.h log main.cpp proto uicomponents util
SOURCES_mrfcap := main.cpp mrfcap.cpp util
SOURCES_mrftest := drive main.cpp mrf test/common test/mrf uicomponents util
SOURCES_nulltest := drive main.cpp test/common test/null uicomponents util
SOURCES_sdutil := main.cpp sdutil.cpp mrf/constants.cpp util

#
# All the pkg-config packages used.
#
PACKAGES := glibmm-2.4 gtkmm-3.0 cairomm-1.0 ode protobuf libxml++-2.6 gsl libusb-1.0

#
# The flags to pass to the linker ahead of any object files.
#
PROJECT_LDFLAGS := -pthread -Wl,--as-needed -Wl,-O2 -ggdb3

#
# The library flags to pass to the linker after all object files.
#
PROJECT_LIBS := -lbz2 -L./boost_1_54_0/stage/lib -l:libboost_coroutine.a -l:libboost_context.a -lgtest -lgtest_main

#
# The flags to pass to the C++ compiler.
#
# Use C++11 (can be kicked up to a higher level when ready).
PROJECT_CXXFLAGS := -std=c++11
# Enable threading and thread-safe functions.
PROJECT_CXXFLAGS += -pthread
# Enable lots of warnings.
PROJECT_CXXFLAGS += -Wall -Wextra
# Enable extra warnings about printf/scanf/strftime format strings.
PROJECT_CXXFLAGS += -Wformat=2
# Warn when C-style cast is used in C++ code.
PROJECT_CXXFLAGS += -Wold-style-cast
# Warn when possibly erroneous value-altering implicit conversions are used.
PROJECT_CXXFLAGS += -Werror=conversion
# Warn when attemping to compare between signed and unsigned integers
PROJECT_CXXFLAGS += -Werror=sign-compare
# Warn when an undefined macro name is referenced.
PROJECT_CXXFLAGS += -Wundef
# Warn when a global function has no previous declaration (i.e. missing from the header file or should be file-scope).
PROJECT_CXXFLAGS += -Wmissing-declarations
# Warn when the same thing is declared more than once, pointlessly.
PROJECT_CXXFLAGS += -Wredundant-decls
# Warn when casting from a type to itself.
PROJECT_CXXFLAGS += -Wuseless-cast
# Warn when using 0 as a null pointer constant; nullptr is preferable.
PROJECT_CXXFLAGS += -Wzero-as-null-pointer-constant
# Warn when doing pointer arithmetic on null, void, or function pointers.
PROJECT_CXXFLAGS += -Wpointer-arith
# Optimize reasonably well for the native machine.
PROJECT_CXXFLAGS += -march=native -O2
# Do not allow multiple definitions of the same global variable.
PROJECT_CXXFLAGS += -fno-common
# Optimize by assuming nobody checks errno for math functions.
PROJECT_CXXFLAGS += -fno-math-errno
# Optimize by assuming pointers do not alias when they are not permitted to do so according to type.
PROJECT_CXXFLAGS += -fstrict-aliasing
# Include full debug information.
PROJECT_CXXFLAGS += -ggdb3
# Define strerror_r.
PROJECT_CXXFLAGS += -D_POSIX_C_SOURCE=200112L
# If building on a 32-bit platform, nevertheless use 64-bit offsets when seeking in files.
PROJECT_CXXFLAGS += -D_FILE_OFFSET_BITS=64
# Define the [U]INT[N]_C macros in <cinttypes>.
PROJECT_CXXFLAGS += -D__STDC_CONSTANT_MACROS=1
# Use inline functions where available from GNU Scientific Library.
PROJECT_CXXFLAGS += -DHAVE_INLINE
# Use boost includes from the local directory, rather then from a system 
# directory, as we use a depreceated version of boost
PROJECT_CXXFLAGS += -I ./boost_1_54_0
# Take include files from the project root directory rather than relative to the source file.
PROJECT_CXXFLAGS += -I.

#
# Runs the unit tests.
#
.PHONY : test
test : bin/test
	bin/test
