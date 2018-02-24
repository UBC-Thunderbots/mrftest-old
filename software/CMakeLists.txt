
# set the path to the .cmake files
set(CMAKE_TOOLS cmake_tools)

# set our include directory so that the files know where to
# look for headers
include_directories(${CMAKE_SOURCE_DIR}/software)
# include the get_packages.cmake file to load third party libs
include(${CMAKE_TOOLS}/get_packages.cmake)

# enable test for gtest
enable_testing()

# a list of all the executables we want to compile
set(ALL_PROJECTS "ai" "mrftest" "buildid" "log" "getcore" "hall2phase" "mrfcap" "nulltest" "sdutil" "software_test")

# loop through each executable
foreach(proj ${ALL_PROJECTS})
    # name the target project, which will be the name of the executable
    project(${proj})
    # include the code from the corresponding file's .cmake file that will load its source files
    include(${CMAKE_TOOLS}/${proj}.cmake)
    # add the source files and link the libraries
    include(${CMAKE_TOOLS}/create_binary.cmake)
endforeach(proj)


