cmake_minimum_required(VERSION 3.7)

# project name
project(software)
# set the path to the .cmake tools
set(CMAKE_TOOLS "${CMAKE_CURRENT_SOURCE_DIR}/cmake_tools")

# set the path to the .cmake binary builders
set(CMAKE_BINARY_BUILDERS "${CMAKE_CURRENT_SOURCE_DIR}/cmake_binary_builders")

# include the search function
include("${CMAKE_TOOLS}/search.cmake")

# makes the bin directory to store the binaries in
file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")

# set our include directory so that the files know where to
# look for headers
include_directories("${CMAKE_CURRENT_SOURCE_DIR}")

# include the get_packages.cmake file to load third party libs
include("${CMAKE_TOOLS}/get_packages.cmake")

# add the compiler definitions
include("${CMAKE_TOOLS}/software_definitions.cmake")

# all the libraries you will need to link against if you are using the ./util folder
set(UTIL_LIBRARIES
    "${LibXML++_LIBRARIES}"
    "${GTKMM_LIBRARIES}"
    "${BZIP2_LIBRARIES}"
    "${PROTOBUF_LIBRARIES}"
    "${GLIB_LIBRARIES}"
    "${LIBUSB_1_LIBRARIES}"
    "${GSL_LIBRARIES}")


# common file patterns that are used amongst the executables. We include headers here to allow for full code
# completion across all of our source files.
set(COMMON_PATTERNS "*.cpp" "*.h")

# Build mrftest
set(binary_name "mrftest") 
set(binary_cmake "${CMAKE_BINARY_BUILDERS}/${binary_name}.cmake") 
include("${binary_cmake}")
build_specific_binary("${binary_name}")
# set cmake to put the executable in the given directory
set_target_properties(
        "${binary_name}"
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")
