


function(build_specific_binary "binary")
    # the folders where the source files are
    set(SOURCE_FOLDERS "hall2phase")
    # the file names to match
    set(PATTERNS "*.cpp")

    # get the source files
    search("${PATTERNS}" "${SOURCE_FOLDERS}" "src")

    # add the source files
    add_executable(${binary} "${src}")

    # link against libraries
    target_link_libraries(${binary}
            "${LibXML++_LIBRARIES}"
            "${GTKMM_LIBRARIES}"
            "${BZIP2_LIBRARIES}"
            "${PROTOBUF_LIBRARIES}"
            "${GLIB_LIBRARIES}"
            "${LIBUSB_1_LIBRARIES}"
            "${GSL_LIBRARIES}"
            )
endfunction(build_specific_binary)
