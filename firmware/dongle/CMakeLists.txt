
# we have an individual CMakeLists.txt for dongle here so that we can avoid include conflicts with robot firmware

# set the name of the dongle binary without the extension
set(DONGLE_BINARY "dongle_firmware")

# get all of the source files
file(GLOB_RECURSE dongle-src
        "${FIRMWARE_SOURCE_DIR}/usb/*.c"
        "${FIRMWARE_SOURCE_DIR}/stm32lib/*.c"
        "${FIRMWARE_SOURCE_DIR}/freertos/source/*.c"
        "${FIRMWARE_SOURCE_DIR}/dongle/*.c"
        )
# include the headers from dongle
include_directories("${FIRMWARE_SOURCE_DIR}/dongle")

# creates the .bin, .elf, and .map files
# this function is included in firmware/CMakeLists.txt which this CMakeLists.txt file is a subdirectory of
build_firmware("${FIRMWARE_SOURCE_DIR}" "${dongle-src}" "${DONGLE_BINARY}" "dongle")

##### BUILD FOR DFUSE #####
add_custom_target(${DONGLE_BINARY}.dfu)
include("${FIRMWARE_SOURCE_DIR}/cmake_binary_builders/dfu.cmake")
flash("${DONGLE_BINARY}" "dfu")

