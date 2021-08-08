file(COPY ${CMAKE_SOURCE_DIR}/.clang-format 
        DESTINATION ${CMAKE_BINARY_DIR})

find_program(CLANG_FORMAT
	clang-format
	NAMES clang-format-13 clang-format-13 clang-format-12 clang-format-11 
          clang-format13 clang-format13 clang-format12 clang-format11 
	REQUIRED)

set(TESTS
	test_type
	test_object
)

foreach(TEST_NAME ${TESTS})
	set(TEST_BIN ${TEST_NAME})
	set(TEST_SRC "${TEST_NAME}.cc")
	set(TEST_HEADER "${TEST_NAME}.h")
	set(TEST_EXPECTED "${TEST_NAME}.conf.expected")
	add_custom_command(OUTPUT ${TEST_HEADER}
		COMMAND config-gen "-o" ${TEST_HEADER} "-e" "${CMAKE_CURRENT_SOURCE_DIR}/${TEST_NAME}.conf"
		COMMENT "Generating test header ${TEST_HEADER}"
		MAIN_DEPENDENCY "${TEST_NAME}.conf")
	if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${TEST_SRC}")
		add_executable(${TEST_BIN} ${TEST_SRC} "${CMAKE_CURRENT_BINARY_DIR}/${TEST_HEADER}")
		target_include_directories(${TEST_BIN} PRIVATE ${UCL_INCLUDE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR})
		target_link_libraries(${TEST_BIN} PRIVATE ${UCL_LIBRARY})
		add_test(NAME ${TEST_BIN} COMMAND ${TEST_BIN})
	endif()
endforeach()
