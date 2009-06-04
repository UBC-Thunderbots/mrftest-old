cmake_minimum_required(VERSION 2.6)

find_package(PkgConfig REQUIRED)

pkg_check_modules(GTKMM REQUIRED gtkmm-2.4)

include_directories(${GTKMM_INCLUDE_DIRS})
include_directories(include)

add_subdirectory(src)

