
function(build_specific_binary binary_name)
    # the folders where the source files are
    set(SOURCE_FOLDERS "drive" "mrf" "test/common" "test/mrf" "uicomponents" "util")
    # the file names to match
    set(PATTERNS "*.cpp")

    # get the source files
    search("${PATTERNS}" "${SOURCE_FOLDERS}" "src")
    list(APPEND "src" "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")

    # add the source files
    add_executable(${binary_name} "${src}")

    # link against libraries
    target_link_libraries(${binary_name} "${UTIL_LIBRARIES}")
endfunction(build_specific_binary)
