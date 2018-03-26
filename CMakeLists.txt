# minimum CMake version
# using version 3.7 so that it works with gitlab ci
cmake_minimum_required(VERSION 3.7)

# project name
project(thunderbots)

# add our firmware and software subdirectories so that CMake knows they have CMakeLists.txt files
add_subdirectory(firmware)
add_subdirectory(software)
add_subdirectory(tools/fwsim)
