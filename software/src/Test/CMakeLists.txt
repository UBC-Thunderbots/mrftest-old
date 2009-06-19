file(GLOB TEST_SRC *.cpp)

add_executable(test ${TEST_SRC})

target_link_libraries(test datapool Log UI XBeeHost ${GTKMM_LIBRARIES} -lrt)
