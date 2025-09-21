function(AddTest NAME EXEC_TARGET)
	cmake_parse_arguments(PARSE_ARGV 0 "test" "" "" "FILTERS")
	add_test(NAME ${NAME}
		COMMAND $<TARGET_FILE:${EXEC_TARGET}> ${test_FILTERS}
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	)
endfunction(AddTest NAME EXEC_TARGET)
