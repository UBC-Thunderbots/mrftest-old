
set(SOURCE_FOLDERS log proto uicomponents util)
set(AI_COMMON "playtype.*" "colour.h")
set(${PROJECT_NAME}-src)

include(${PROJECT_SOURCE_DIR}/${CMAKE_TOOLS}/search.cmake)
list(APPEND ${PROJECT_NAME}-src ${CMAKE_SOURCE_DIR}/software/main.cpp)

set(AI_COMMON_LOC ${PROJECT_SOURCE_DIR}/ai/common)
foreach(file ${AI_COMMON})
    file(GLOB file-src ${AI_COMMON_LOC}/${file})
    list(APPEND ${PROJECT_NAME}-src ${file-src})
endforeach(file)