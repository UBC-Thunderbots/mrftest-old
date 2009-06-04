file(GLOB AI_SRC *.cpp Strategies/*.cpp)

include_directories(${GTKMM_INCLUDE_DIRS})

add_library(AI ${AI_SRC})

target_link_libraries(AI ${GTKMM_LIBRARIES})
