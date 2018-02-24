# add the source files
add_executable(
        ${PROJECT_NAME}
        ${${PROJECT_NAME}-src}
        ${PROTO_SRCS}
        ${PROTO_HDRS}
)

# link against libraries
target_link_libraries(${PROJECT_NAME}
        ${Boost_LIBRARIES}
        ${LibXML++_LIBRARIES}
        ${GTKMM_LIBRARIES}
        ${BZIP2_LIBRARIES}
        ${PROTOBUF_LIBRARIES}
        ${GLIB_LIBRARIES}
        ${GTEST_BOTH_LIBRARIES}
        ${CMAKE_THREAD_LIBS_INIT}
        ${LIBUSB_1_LIBRARIES}
        ${GSL_LIBRARIES}
        ${ODE_LIBRARY}
        )

# set cmake to put the executable in the given directory
set_target_properties(
        ${PROJECT_NAME}
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/software/bin
)