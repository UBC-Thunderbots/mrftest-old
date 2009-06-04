file(GLOB UI_SRC *.cpp)

add_library(UI ${UI_SRC})

target_link_libraries(UI)

