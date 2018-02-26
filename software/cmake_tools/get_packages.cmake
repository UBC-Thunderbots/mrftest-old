

# Use PkgConfig to find some packages using pkg_check_modules
find_package(PkgConfig REQUIRED)


##### BOOST #####
set(BOOST_LOC "${CMAKE_CURRENT_SOURCE_DIR}/boost_1_54_0")
# IMPORTANT: must link coroutines before context, the order matters here
set(BOOST_LIBRARIES
        "${BOOST_LOC}/stage/lib/libboost_coroutine.a"
        "${BOOST_LOC}/stage/lib/libboost_context.a")
include_directories("${BOOST_LOC}")

##### PROTOBUF #####
# find the Protobuf package
find_package("Protobuf" REQUIRED)
# include Protobuf directories
include_directories("${PROTOBUF_INCLUDE_DIRS}")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/proto")
# get the .proto files
file(GLOB "proto-files" "${CMAKE_CURRENT_SOURCE_DIR}/proto/*.proto")
# get existing generated files so we can clean them
file(GLOB "EXISTING_PROTO" "${CMAKE_CURRENT_SOURCE_DIR}/proto/*.pb.cc" "${CMAKE_CURRENT_SOURCE_DIR}/proto/*.pb.h")
# Generate the protobuf files
# They are generated when CMake generates the Makefiles
# they are stored in software/proto
execute_process(
        COMMAND rm ${EXISTING_PROTO}
        COMMAND protoc ${proto-files} --proto_path=${CMAKE_CURRENT_SOURCE_DIR}/proto --cpp_out=${CMAKE_CURRENT_SOURCE_DIR}/proto)

##### GTK ######
pkg_check_modules("GTKMM" REQUIRED "gtkmm-3.0")
# include the GTK dependencies
include_directories("${GTKMM_INCLUDE_DIRS}")
link_directories("${GTKMM_LIBRARY_DIRS}")


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
pkg_search_module("LibXML++" REQUIRED "libxml++-2.6")
include_directories("${LibXML++_INCLUDE_DIRS}")


##### ODE #####
pkg_search_module("ODE" REQUIRED ode)


##### GSL #####
find_package("GSL" REQUIRED "gsl")
include_directories("${GSL_INCLUDE_DIRS}")


##### CAIRO #####
pkg_search_module("CAIROMM" REQUIRED "cairomm-1.0")
include_directories(${CAIROMM_INCLUDE_DIRS})


##### BZIP2 #####
find_package("BZip2" REQUIRED "bz2")
include_directories("${BZIP2_INCLUDE_DIR}")



