

function(build_specific_binary "binary")
    # the folders where the source files are
    set(SOURCE_FOLDERS "log" "proto" "uicomponents" "util")
    # the file names to match
    set(PATTERNS "*.cpp")

    # get the source files
    search("${PATTERNS}" "${SOURCE_FOLDERS}" "src")

    # do another search for just playtype.* files in ai/common
    set(SOURCE_FOLDERS "ai/common")
    set(PATTERNS "playtype.*")
    search("${PATTERNS}" "${SOURCE_FOLDERS}" "ai_common_files")

    list(APPEND "src" "${ai_common_files}" "${SOFTWARE_SOURCE_DIR}/main.cpp")

    # add the source files
    add_executable(${binary} "${src}" "${PROTO_SRCS}" "${PROTO_HDRS}")

    # link against libraries
    target_link_libraries(${binary}
            "${UTIL_LIBRARIES}"
            "${LIBUSB_1_LIBRARIES}")
endfunction(build_specific_binary)
