file(GLOB TEST_SRC *.cpp)

add_executable(test ${TEST_SRC})

target_link_libraries(test datapool XBeeHost Log UI ${GTKMM_LIBRARIES} -lrt)
