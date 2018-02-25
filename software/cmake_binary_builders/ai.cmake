
function(build_specific_binary "binary")
    # the folders where the source files are
    set(SOURCE_FOLDERS "ai" "drive" "geom" "log/shared" "mrf" "proto" "uicomponents" "util")
    # the file names to match
    set(PATTERNS "*.cpp")

    # get the source files
    search("${PATTERNS}" "${SOURCE_FOLDERS}" "src")
    list(APPEND "src" "${SOFTWARE_SOURCE_DIR}/main.cpp")

    # add the source files
    add_executable(
            ${binary}
            "${src}"
            "${PROTO_SRCS}"
            "${PROTO_HDRS}"
    )

    # link against libraries
    target_link_libraries(${binary}
            "${BOOST_LIBRARIES}"
            "${UTIL_LIBRARIES}")
endfunction(build_specific_binary)

