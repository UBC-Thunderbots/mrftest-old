
function(build_specific_binary "binary")
    # the folders where the source files are
    set(SOURCE_FOLDERS "test/unit-tests" "geom" "util")
    # the file names to match
    set(PATTERNS "*.cpp" "param.*" "string.*" "config.*" "exception.*" "misc.*" "dprint.*" "hungarian.*" "matrix.*" "codec.*")

    # get the source files
    search("${PATTERNS}" "${SOURCE_FOLDERS}" "src")

    # add the source files
    add_executable(${binary} "${src}")

    # link against libraries
    target_link_libraries(${binary}
            "${UTIL_LIBRARIES}"
            "${GTEST_BOTH_LIBRARIES}"
            "${CMAKE_THREAD_LIBS_INIT}")
endfunction(build_specific_binary)
