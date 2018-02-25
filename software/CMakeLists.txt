
# builds the given binary
function(build_binary "binary")
    include("${CMAKE_TOOLS}/search.cmake")
    # include the code from the corresponding file's .cmake file that we can call the binary-specific
    # build_specific_binary function that is defined in each .cmake file
    include("${CMAKE_TOOLS}/${binary}.cmake")
    build_specific_binary("${binary}")
    # set cmake to put the executable in the given directory
    set_target_properties(
            "${binary}"
            PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${SOFTWARE_SOURCE_DIR}/bin"
    )
endfunction(build_binary)

set(SOFTWARE_SOURCE_DIR "${PROJECT_SOURCE_DIR}/software")

# set the path to the .cmake files
set(CMAKE_TOOLS "${SOFTWARE_SOURCE_DIR}/cmake_tools")

# makes the bin directory to store the binaries in
file(MAKE_DIRECTORY "${CMAKE_SOURCE_DIR}/software/bin")

# set our include directory so that the files know where to
# look for headers
include_directories("${CMAKE_SOURCE_DIR}/software")

# include the get_packages.cmake file to load third party libs
include("${CMAKE_TOOLS}/get_packages.cmake")

# a list of all the executables we want to compile
set(ALL_PROJECTS "ai" "mrftest" "software_test")# "buildid" "log" "getcore" "hall2phase" "mrfcap" "nulltest" "sdutil" "software_test")

# loop through each executable
# don't put double quotes around the list we are iterating through
foreach("binary" ${ALL_PROJECTS})
    build_binary("${binary}")
endforeach("binary")




