


function(build_specific_binary binary_name)
    # the folders where the source files are
    set(SOURCE_FOLDERS "hall2phase")
    # the file names to match
    set(PATTERNS "*.cpp")

    # get the source files
    search("${PATTERNS}" "${SOURCE_FOLDERS}" "src")

    # add the source files
    add_executable(${binary_name} "${src}")

    # link against libraries
    target_link_libraries(${binary_name})
endfunction(build_specific_binary)
