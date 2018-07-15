
function(build_specific_binary binary_name)
    # the folders where the source files are
    set(SOURCE_FOLDERS "test/unit-tests" "geom" "util")
    # the file names to match
    set(PATTERNS "${COMMON_PATTERNS} param.*" "string.*" "config.*" "exception.*" "misc.*" "dprint.*" "hungarian.*" "matrix.*" "codec.*")

    # get the source files
    search("${PATTERNS}" "${SOURCE_FOLDERS}" "src")

    # add the source files
    add_executable(${binary_name} "${src}")

    # link against libraries
    target_link_libraries(${binary_name}
            "${UTIL_LIBRARIES}"
            "${GTEST_BOTH_LIBRARIES}"
            "${CMAKE_THREAD_LIBS_INIT}")
endfunction(build_specific_binary)
