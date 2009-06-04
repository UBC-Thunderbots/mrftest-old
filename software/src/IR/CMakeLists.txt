file(GLOB IR_SRC *.cpp)
set(PROTO_FILES
	messages_robocup_ssl_detection
	messages_robocup_ssl_geometry
	messages_robocup_ssl_wrapper)
set(PROTO_OUT_CPPS)

foreach(PROTO_FILE ${PROTO_FILES})
	set(CC_FILE ${PROTO_FILE}.pb.cc)
	set(H_FILE ${PROTO_FILE}.pb.h)
	set(P_FILE ${PROTO_FILE}.proto)

	add_custom_command(OUTPUT ${CC_FILE} ${H_FILE}
		COMMAND protoc --cpp_out=${CMAKE_CURRENT_BINARY_DIR} --proto_path=${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/${P_FILE}
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${P_FILE}
		VERBATIM)

	list(APPEND PROTO_OUT_CPPS ${CMAKE_CURRENT_BINARY_DIR}/${CC_FILE})
endforeach(PROTO_FILE)

include_directories(${CMAKE_CURRENT_BINARY_DIR}/..)

add_library(IR ${IR_SRC} ${PROTO_OUT_CPPS})

target_link_libraries(IR -lprotobuf)

