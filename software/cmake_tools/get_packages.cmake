

# Use PkgConfig to find some packages using pkg_check_modules
find_package(PkgConfig REQUIRED)


##### BOOST #####
set(BOOST_LOC "${CMAKE_SOURCE_DIR}/software/boost_1_54_0")
# IMPORTANT: must link coroutines before context, the order matters here
set(BOOST_LIBRARIES
        "${BOOST_LOC}/stage/lib/libboost_coroutine.a"
        "${BOOST_LOC}/stage/lib/libboost_context.a")
include_directories("${CMAKE_SOURCE_DIR}/software/boost_1_54_0")


##### PROTOBUF #####
# find the Protobuf package
find_package("Protobuf" REQUIRED)
# include Protobuf directories
include_directories("${PROTOBUF_INCLUDE_DIRS}")
include_directories("${CMAKE_CURRENT_BINARY_DIR}")
# get the .proto files
file(GLOB "proto-files" "${CMAKE_SOURCE_DIR}/software/proto/*.proto")
# Generate the protobuf files
# PROTO_SRCS: the *.pb.cc files
# PROTO_HDRS: the *.pb.h files
# they are stored in cmake-build-debug/software
PROTOBUF_GENERATE_CPP("PROTO_SRCS" "PROTO_HDRS" "${proto-files}")


##### GTK ######
pkg_check_modules("GTKMM" REQUIRED "gtkmm-3.0")
# include the GTK dependencies
include_directories("${GTKMM_INCLUDE_DIRS}")
link_directories("${GTKMM_LIBRARY_DIRS}")
# add the GTK flags
add_definitions("${GTKMM_CFLAGS_OTHER}")


##### GTEST #####
# enable testing, find the GTest package, and include its directories
find_package("GTest" REQUIRED)
include_directories("${GTEST_INCLUDE_DIRS}")


##### GLIB #####
pkg_search_module("GLIBMM" REQUIRED "glibmm-2.4")
include_directories("${GLIBMM_INCLUDE_DIRS}")


##### LIBUSB #####
pkg_search_module("LIBUSB_1" REQUIRED "libusb-1.0")
include_directories("${LIBUSB_1_INCLUDE_DIRS}")
link_directories("${LIBUSB_1_LIBRARY_DIRS}")
add_definitions("${LIBUSB_1_DEFINITIONS}")


##### LIBXML #####
pkg_search_module("LibXML++" REQUIRED libxml++-2.6)
include_directories("${LibXML++_INCLUDE_DIRS}")


##### ODE #####
pkg_search_module("ODE" REQUIRED)
include_directories("${ODE_INCLUDE_DIR}")


##### GSL #####
find_package("GSL" REQUIRED)
include_directories("${GSL_INCLUDE_DIRS}")


##### CAIRO #####
pkg_search_module("CAIROMM" REQUIRED "cairomm-1.0")
include_directories(${CAIROMM_INCLUDE_DIRS})


##### BZIP2 #####
find_package("BZip2" REQUIRED)
include_directories("${BZIP2_INCLUDE_DIR}")



