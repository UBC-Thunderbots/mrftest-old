
set(SOURCE_FOLDERS "test/unit-tests" "geom")
set(UTIL_FILES "param.*" "string.*" "config.*" "exception.*" "misc.*" "dprint.*" "hungarian.*" "matrix.*" codec.*)
set(${PROJECT_NAME}-src)


include(${PROJECT_SOURCE_DIR}/${CMAKE_TOOLS}/search.cmake)

foreach(file ${UTIL_FILES})
    file(GLOB folder-src ${CMAKE_SOURCE_DIR}/software/util/${file} ${CMAKE_SOURCE_DIR}/software/util/${file})
    list(APPEND ${PROJECT_NAME}-src ${folder-src})
endforeach(file)
