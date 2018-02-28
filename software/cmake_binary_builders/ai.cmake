
function(build_specific_binary binary_name)
    # the folders where the source files are
    set(SOURCE_FOLDERS "ai" "drive" "geom" "log/shared" "mrf" "proto" "uicomponents" "util")
    # the file names to match
    set(PATTERNS "*.cpp")

    # get the source files
    search("${PATTERNS}" "${SOURCE_FOLDERS}" "src")
    list(APPEND "src" "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")

    file(GLOB PROTO_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/proto/*.pb.cc")
#    file(GLOB PROTO_HDRS "${CMAKE_CURRENT_SOURCE_DIR}/proto/*.pb.cc")

    # add the source files
    add_executable(
            ${binary_name}
            "${src}"
            "${PROTO_SRCS}"
    )

    # link against libraries
    target_link_libraries(${binary_name}
            "${BOOST_LIBRARIES}"
            "${UTIL_LIBRARIES}")


endfunction(build_specific_binary)

