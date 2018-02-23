# you should assign a variable using set(SOURCE_FOLDERS ...) so that we can loop through it here to get all of the
# .cpp and .h files then append them to the ${PROJECT_NAME}-src variable
foreach(folder ${SOURCE_FOLDERS})
    file(GLOB_RECURSE folder-src
            ${CMAKE_SOURCE_DIR}/software/${folder}/*.cpp
            ${CMAKE_SOURCE_DIR}/software/${folder}/*.h)
    list(APPEND ${PROJECT_NAME}-src ${folder-src})
endforeach(folder)