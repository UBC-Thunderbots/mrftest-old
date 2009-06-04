file(GLOB LEASTSQUARES_SRC *.cpp)
include_directories(.)
add_library(leastsquares ${LEASTSQUARES_SRC})
