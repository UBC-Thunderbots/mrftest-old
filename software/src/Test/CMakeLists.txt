file(GLOB TEST_SRC *.cpp)

add_executable(test ${TEST_SRC})

target_link_libraries(test UI XBeeHost Log datapool ${GTKMM_LIBRARIES} -lrt)
