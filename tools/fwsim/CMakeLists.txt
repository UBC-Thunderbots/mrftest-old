
##### CMake for fwsim #####

cmake_minimum_required(VERSION 3.7)

# the name of our project
project(fwsim)

# the name of the output binary
set(TARGET_NAME "sim")

# the location of the firmware code
set(FIRMWARE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../firmware")

# tells the compiler to ignore code wrapped in #ifndef FWSIM #endif blocks
add_definitions(-DFWSIM)

# set some variables of files we need to use. They are separated here into the locations that the files exist in
# ${FIRMWARE_SOURCE_DIR}/main/primitives
set(_PRIMITIVES "primitive" "move" "shoot" "spin" "pivot" "jcatch" "stop" "dribble")
# ${FIRMWARE_SOURCE_DIR}/main
set(_MAIN "physics" "simulate" "control" "bangbang" "wheels" "dribbler")
# ${FIRMWARE_SOURCE_DIR}/main/util
set(_UTIL "physbot" "util" "log")

# this appends the given path to each of the files given and attaches a .c extension onto them, then returns the list
# with the given out_name
function(create_executable_paths path files out_name)
    set(temp)
    foreach("file" ${files})
        list(APPEND temp "${path}/${file}.c")
    endforeach("file")
    set(${out_name} ${temp} PARENT_SCOPE)
endfunction(create_executable_paths)

# build the paths to each of our source files
create_executable_paths("${FIRMWARE_SOURCE_DIR}/main/primitives" "${_PRIMITIVES}" "PRIMITIVES")
create_executable_paths("${FIRMWARE_SOURCE_DIR}/main" "${_MAIN}" "MAIN")
create_executable_paths("${FIRMWARE_SOURCE_DIR}/main/util" "${_UTIL}" "UTIL")

# set the includes locations
include_directories("${FIRMWARE_SOURCE_DIR}/main")
include_directories("${FIRMWARE_SOURCE_DIR}/main/primitives")

# create the executable
add_executable("${TARGET_NAME}"
        "${PRIMITIVES}"
        "${MAIN}"
        "${UTIL}"
        "${CMAKE_CURRENT_SOURCE_DIR}/main.c"
        "${FIRMWARE_SOURCE_DIR}/main/primitives/primitive.h")

# tell the compiler to use gnu99
target_compile_options(${TARGET_NAME} PUBLIC "-std=gnu99")

# link against the math library
target_link_libraries(${TARGET_NAME} "m")

# tell CMake to store the binary in the tools/fwsim directory
set_target_properties(
        "${TARGET_NAME}"
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")