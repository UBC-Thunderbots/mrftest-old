add_executable(test test.cpp)

target_link_libraries(test datapool XBeeHost Log UI ${GTKMM_LIBRARIES} -lrt)
