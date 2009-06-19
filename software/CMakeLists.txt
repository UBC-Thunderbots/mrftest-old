project(thunderbots)

cmake_minimum_required(VERSION 2.6)

set(CMAKE_C_FLAGS "-Wall -Wextra -march=native -mfpmath=sse")
set(CMAKE_CXX_FLAGS "-Wall -Wextra -march=native -mfpmath=sse")

find_package(PkgConfig REQUIRED)

pkg_check_modules(SIGCPP REQUIRED sigc++-2.0)
include_directories(${SIGCPP_INCLUDE_DIRS})

pkg_check_modules(GLIBMM REQUIRED glibmm-2.4)
include_directories(${GLIBMM_INCLUDE_DIRS})

pkg_check_modules(GTKMM REQUIRED gtkmm-2.4)
include_directories(${GTKMM_INCLUDE_DIRS})

pkg_check_modules(CAIROMM REQUIRED cairomm-1.0)
include_directories(${CAIROMM_INCLUDE_DIRS})

include_directories(include)
add_subdirectory(src)

