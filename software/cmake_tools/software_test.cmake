


function(build_specific_binary "binary")
    set(SOURCE_FOLDERS "test/unit-tests" "geom" "util")
    set(PATTERNS "*.cpp" "param.*" "string.*" "config.*" "exception.*" "misc.*" "dprint.*" "hungarian.*" "matrix.*" "codec.*")
    search("${PATTERNS}" "${SOURCE_FOLDERS}")

    # add the source files
    add_executable(${binary} "${src}")

    # link against libraries
    target_link_libraries(${binary}
            #            ${BOOST_LIBRARIES}
            "${LibXML++_LIBRARIES}"
            "${GTKMM_LIBRARIES}"
            "${BZIP2_LIBRARIES}"
            "${PROTOBUF_LIBRARIES}"
            "${GLIB_LIBRARIES}"
            "${GTEST_BOTH_LIBRARIES}"
            "${CMAKE_THREAD_LIBS_INIT}"
            "${LIBUSB_1_LIBRARIES}"
            "${GSL_LIBRARIES}"
            #            ${ODE_LIBRARY}
            )
endfunction(build_specific_binary)
