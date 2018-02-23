

# Set the location of the Findxx.cmake files
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

# Use PkgConfig to find some packages using pkg_check_modules
find_package(PkgConfig REQUIRED)

##### BOOST #####
# need to run
# sudo apt-get install cmake libblkid-dev e2fslibs-dev libboost-all-dev libaudit-dev
# for boost to work
find_package(Boost 1.54.0 REQUIRED
        COMPONENTS
        system
        unit_test_framework
        filesystem
        coroutine
        context)
include_directories(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIRS} )
# need this to remove error that boost raises
add_definitions(-DBOOST_ASIO_SEPARATE_COMPILATION)

##### PROTOBUF #####
# proto files
# find the Protobuf package
find_package(Protobuf REQUIRED)

# include Protobuf directories
include_directories(${PROTOBUF_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# get the .proto files
file(GLOB proto-files ${CMAKE_SOURCE_DIR}/software/proto/*.proto)
# Generate the protobuf files
# PROTO_SRCS: the *.pb.cc files
# PROTO_HDRS: the *.pb.h files
# they are stored in cmake-build-debug/${PROJECT_NAME}
PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${proto-files})

##### GTK ######
pkg_check_modules(REQUIRED gtkmm-3.0)
# include the GTK dependencies
include_directories(${GTKMM_INCLUDE_DIRS})
link_directories(${GTKMM_LIBRARY_DIRS})
# add the GTK flags
add_definitions(${GTKMM_CFLAGS_OTHER})

##### GTEST #####
# enable testing, find the GTest package, and include its directories
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

##### GLIB #####
pkg_check_modules(REQUIRED glibmm-2.4)
include_directories(${GLIB_INCLUDE_DIRS})



##### LIBUSB #####
pkg_check_modules(REQUIRED libusb-1.0)
include_directories(${LIBUSB_1_INCLUDE_DIRS})
link_directories(${LIBUSB_1_LIBRARY_DIRS})
add_definitions(${LIBUSB_1_DEFINITIONS})

##### LIBXML #####
find_package(LibXML++ REQUIRED)
include_directories(${LibXML++_INCLUDE_DIRS})


##### ODE #####
find_package(ODE REQUIRED)
include_directories(${ODE_INCLUDE_DIR})

##### GSL #####
find_package(GSL REQUIRED)
include_directories(${GSL_INCLUDE_DIRS})

##### CAIRO #####
pkg_check_modules(REQUIRED cairomm-1.0)
include_directories(/usr/include)

##### BZIP2 #####
find_package(BZip2 REQUIRED)
include_directories(${BZIP2_INCLUDE_DIR})

##### strerror_r #####
add_definitions(-D_POSIX_C_SOURCE=200112L)
##### toggle for 32/64 bit offsets #####
add_definitions(-D_FILE_OFFSET_BITS=64)
##### Define the [U]INT[N]_C macros in <cinttypes>#####
add_definitions(-D__STDC_CONSTANT_MACROS=1)
##### Use inline functions where available from GNU Scientific Library #####
add_definitions(-DHAVE_INLINE)
# Optimize reasonably well for the native machine.
add_definitions(-march=native -O2)
# Do not allow multiple definitions of the same global variable.
add_definitions(-fno-common)
# Optimize by assuming nobody checks errno for math functions.
add_definitions(-fno-math-errno)
# Optimize by assuming pointers do not alias when they are not permitted to do so according to type.
add_definitions(-fstrict-aliasing)
# Include full debug information.
add_definitions(-ggdb3)
# use c++11
add_definitions(-std=c++11)
# use pthread
add_definitions(-pthread)

add_definitions(-Wl,--as-needed)
add_definitions(-Wl,-02)
add_definitions(-Wall)

