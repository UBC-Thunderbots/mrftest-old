

function(build_specific_binary binary_name)
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

    list(APPEND "src" "${ai_common_files}" "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")

    file(GLOB PROTO_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/proto/*.pb.cc")

    # add the source files
    add_executable(${binary_name} "${src}" "${PROTO_SRCS}")

    # link against libraries
    target_link_libraries(${binary_name}
            "${UTIL_LIBRARIES}"
            "${LIBUSB_1_LIBRARIES}")
endfunction(build_specific_binary)
