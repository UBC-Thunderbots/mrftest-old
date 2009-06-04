add_subdirectory(AI)
add_subdirectory(datapool)
add_subdirectory(IR)
add_subdirectory(XBee)
add_subdirectory(test)
add_subdirectory(leastsquares)
add_subdirectory(Log)
add_subdirectory(UI)
add_subdirectory(xbeewipe)

add_executable(thunderbots Thunderbots.cpp)

target_link_libraries(thunderbots ${GTKMM_LIBRARIES})
target_link_libraries(thunderbots AI datapool IR XBeeHost leastsquares Log UI -lrt)
