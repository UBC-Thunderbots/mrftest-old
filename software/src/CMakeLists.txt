add_subdirectory(AI)
add_subdirectory(datapool)
add_subdirectory(IR)
add_subdirectory(XBee)
add_subdirectory(Test)
add_subdirectory(leastsquares)
add_subdirectory(Log)
add_subdirectory(UI)
add_subdirectory(xbeewipe)

add_executable(thunderbots Thunderbots.cpp)

target_link_libraries(thunderbots ${GTKMM_LIBRARIES})
target_link_libraries(thunderbots IR AI XBeeHost Log UI datapool leastsquares -lrt)
