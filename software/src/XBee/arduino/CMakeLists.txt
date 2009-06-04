project(Arduino)

set(SERIAL_PORT /dev/ttyUSB0)

set(librariesFolder ${CMAKE_SOURCE_DIR}/hardware/libraries)
set(coreFolder ${CMAKE_SOURCE_DIR}/hardware/cores/arduino)
set(sketchbookFolder ${CMAKE_SOURCE_DIR}/sketchbook)

set(MCU atmega168)
set(F_CPU 16000000L)

#binaries (tools)
set(CMAKE_C_COMPILER avr-gcc)
set(CMAKE_CXX_COMPILER avr-g++)
set(CMAKE_AR avr-ar)
set(CMAKE_RANLIB avr-ranlib)
set(CMAKE_OBJCOPY avr-objcopy)
set(AVRDUDE avrdude)
set(UISP uisp)

#build args
set(CMAKE_C_FLAGS "-g -Os -mmcu=${MCU} -DF_CPU=${F_CPU} -DCPU_FREQ=${F_CPU}")
set(CMAKE_CXX_FLAGS "-g -Os -mmcu=${MCU} -DF_CPU=${F_CPU} -DCPU_FREQ=${F_CPU}")
set(CMAKE_EXE_LINKER_FLAGS "-lm")

#include directories
file(GLOB includes ${coreFolder} ${librariesFolder}/* ${librariesFolder}/*/utility)
include_directories(${includes})

#add core
file(GLOB coreFiles ${coreFolder}/*.cpp ${coreFolder}/*.c)
add_library(arduinoCore ${coreFiles})

#add libraries
file(GLOB_RECURSE arduinoLibFiles ${librariesFolder}/*.c *.cpp)
add_library(arduinoLibs ${arduinoLibFiles})
target_link_libraries(arduinoLibs arduinoCore)

#add sketches
FILE(GLOB sketches ${sketchbookFolder}/*.pde ${sketchbookFolder}/*/*.pde)

#preprocessing macro, need to figure out how to add macro to add_custom_command
MACRO(preProcess cppFile pdeFile)
	#preprocessing
	file(WRITE ${cppFile} "#include \"WProgram.h\" \n") 
	file(READ ${pdeFile} pdeSrc) 
	file(APPEND ${cppFile} "${pdeSrc}")
	file(READ ${coreFolder}/main.cxx mainSrc)
	file(APPEND ${cppFile} "${mainSrc}")
ENDMACRO(preProcess)

FOREACH(pdeFile ${sketches})
	get_filename_component(title ${pdeFile} NAME_WE) 
	set(appletFolder ${sketchbookFolder}/${title}/applet)
	set(cppFile ${appletFolder}/${title}.cpp)
	set(elfFile ${title}.elf)
	set(hexFile ${appletFolder}/${title}.hex)
	
	#preprocessing (only run when cmake is called)
	preProcess(${cppFile} ${pdeFile})

	#compile elf	
	add_executable(${elfFile} ${cppFile})
	target_link_libraries(${elfFile} arduinoCore arduinoLibs)
	
	#necessary to get working if want to make 'make' command dependent on changes in pde file 
	add_custom_command(TARGET ${elfFile} 
		PRE_BUILD
		DEPENDS ${pdeFile} 		
		#preProcessing macro should be called here, currently unix-only hack
		COMMAND echo Preprocessing ${pdeFile}
		COMMAND echo #include "WProgram.h" > ${cppFile}
		COMMAND cat ${pdeFile} >> ${cppFile}
		COMMAND cat ${coreFolder}/main.cxx >> ${cppFile})
	
	#create hex file
	add_custom_command(TARGET ${elfFile}
		POST_BUILD
		COMMAND ${CMAKE_OBJCOPY}
    	ARGS -O ihex -R .flash ${elfFile} ${hexFile})

	#create upload target, named upload_<target name>
	add_custom_target(upload_${title}
		COMMAND ${AVRDUDE}
        ARGS    -P${SERIAL_PORT} -pm168 -cstk500v1 -P${SERIAL_PORT} -b19200 -D -Uflash:w:${hexFile}) 

ENDFOREACH(pdeFile)

