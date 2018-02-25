


function(build_specific_binary "binary")
    # the folders where the source files are
    set(SOURCE_FOLDERS "ai" "drive" "geom" "log/shared" "mrf" "proto" "uicomponents" "util")
    # the file names to match
    set(PATTERNS "*.cpp")

    # get the source files
    search("${PATTERNS}" "${SOURCE_FOLDERS}")
    set(src "${src}" "${SOFTWARE_SOURCE_DIR}/main.cpp")

    # add the source files
    add_executable(
            ${binary}
            "${src}"
            "${PROTO_SRCS}"
            "${PROTO_HDRS}"
    )

    # link against libraries
    target_link_libraries(${binary}
            ${BOOST_LIBRARIES}
            ${LibXML++_LIBRARIES}
            ${GTKMM_LIBRARIES}
            ${BZIP2_LIBRARIES}
            ${PROTOBUF_LIBRARIES}
            ${GLIB_LIBRARIES}
#            ${GTEST_BOTH_LIBRARIES}
#            ${CMAKE_THREAD_LIBS_INIT}
            ${LIBUSB_1_LIBRARIES}
            ${GSL_LIBRARIES}
#            ${ODE_LIBRARY}
            )
endfunction(build_specific_binary)

