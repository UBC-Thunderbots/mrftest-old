

# set the path to our software folder
set(SOFTWARE_SOURCE_DIR "${PROJECT_SOURCE_DIR}/software")

# set the path to the .cmake tools
set(CMAKE_TOOLS "${SOFTWARE_SOURCE_DIR}/cmake_tools")

# set the path to the .cmake binary builders
set(CMAKE_BINARY_BUILDERS "${SOFTWARE_SOURCE_DIR}/cmake_binary_builders")

# include the search function
include("${CMAKE_TOOLS}/search.cmake")

# makes the bin directory to store the binaries in
file(MAKE_DIRECTORY "${CMAKE_SOURCE_DIR}/software/bin")

# set our include directory so that the files know where to
# look for headers
include_directories("${CMAKE_SOURCE_DIR}/software")

# include the get_packages.cmake file to load third party libs
include("${CMAKE_TOOLS}/get_packages.cmake")

# add the compiler definitions
include("${CMAKE_TOOLS}/software_definitions.cmake")

# all the libraries you will need to link against if you are using the ${SOFTWARE_SOURCE_DIR}/util folder
set(UTIL_LIBRARIES
    "${LibXML++_LIBRARIES}"
    "${GTKMM_LIBRARIES}"
    "${BZIP2_LIBRARIES}"
    "${PROTOBUF_LIBRARIES}"
    "${GLIB_LIBRARIES}"
    "${LIBUSB_1_LIBRARIES}"
    "${GSL_LIBRARIES}")

# a list of all the executables we want to compile
set(ALL_BINARIES "ai" "mrftest" "software_test" "buildid" "getcore" "hall2phase" "log" "mrfcap" "nulltest" "sdutil")

# loop through each executable
# don't put double quotes around the list we are iterating through
foreach("binary" ${ALL_BINARIES})
    set(binary_cmake "${CMAKE_BINARY_BUILDERS}/${binary}.cmake")
    # check if we have an appropriate cmake file for the given binary
    if (EXISTS "${binary_cmake}")
        # include the code from the corresponding file's .cmake file that we can call the binary-specific
        # build_specific_binary function that is defined in each .cmake file
        include("${binary_cmake}")
        build_specific_binary("${binary}")
        # set cmake to put the executable in the given directory
        set_target_properties(
                "${binary}"
                PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY "${SOFTWARE_SOURCE_DIR}/bin")
    else()
        message(SEND_ERROR "Could not find file ${binary_cmake}.\nPlease make a file named ${binary}.cmake in ${CMAKE_BINARY_BUILDERS}")
    endif()
endforeach("binary")




