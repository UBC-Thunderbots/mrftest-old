file(GLOB LOG_SRC *.cpp)

add_library(Log ${LOG_SRC})

target_link_libraries(Log)

