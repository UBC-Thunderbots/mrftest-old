
set(SOURCE_FOLDERS "drive" "mrf" "test/common" "test/mrf" "uicomponents" "util")
set(${PROJECT_NAME}-src)

include(${PROJECT_SOURCE_DIR}/${CMAKE_TOOLS}/search.cmake)
list(APPEND ${PROJECT_NAME}-src ${CMAKE_SOURCE_DIR}/software/main.cpp)